#include "lib.hpp"

#include "nn/util/util_snprintf.hpp"
#include "nn.hpp"
#include "simpleio.h"
#include "seaddefs.h"

#define PRINT(...)                                         \
	{                                                      \
		int len = nn::util::SNPrintf(buf, sizeof(buf), __VA_ARGS__); \
		svcOutputDebugString(buf, len);                    \
	}

using DecompFunc = u32 (void*, void*, u32, void*, u32);
using GetSizeFunc = size_t(void*, size_t);
using InitFunction = void(void*);

void* g_SSBOChunkDataMgr;
void* g_SSBOQuadDataMgr;
void** g_CaveModule;
DecompFunc* decompressMeshCodec;
DecompFunc* decompressZstd;
GetSizeFunc* ZSTD_readDecompressedSize;
InitFunction* initializeComponents;

sead::Heap* g_Heap;

struct ResChunk {
    u32 hash;
    u32 vertex_output_offset;
    u32 vertex_output_size;
    u32 output_size;
    u32 work_mem_size;
};

void DecompressFile(void* resource, size_t file_size, sead::Heap* heap, const char* out_path, bool is_mc) {
    char buf[0x380];
    if (is_mc) {
        PRINT("Decompressing (MC) to %s", out_path)
        ResChunk* file = reinterpret_cast<ResChunk*>(resource);
        void* out_buf = heap->tryAlloc(file->output_size, 0x1000);
        u32 size = decompressMeshCodec(g_SSBOChunkDataMgr, out_buf, file->output_size, resource, file_size);
        OutputFile(out_path, out_buf, size);
        PRINT("Created file %s", out_path)

        heap->free(out_buf);
    } else {
        PRINT("Decompressing (ZSTD) to %s", out_path)
        size_t decomp_size = ZSTD_readDecompressedSize(resource + 4, file_size - 4);
        void* out_buf = heap->tryAlloc(decomp_size, 0x1000);
        u32 size = decompressZstd(g_SSBOQuadDataMgr, out_buf, decomp_size, resource, file_size);
        OutputFile(out_path, out_buf, size);
        PRINT("Created file %s", out_path)

        heap->free(out_buf);
    }
}

constexpr inline size_t cDecompressedFileBufferSize = 0xD00000;
void DecompressDirectory(const char* in_dir, const char* out_dir, sead::Heap* heap, void* file_buffer_memory, bool skip_mf = true) {

    // MinusField has to come after because for some reason it doesn't like switching from mc to zstd then back
    if (skip_mf && strstr(in_dir, "MinusField") != nullptr) return;
    
    char buf[0x400];
    PRINT("Handling %s", in_dir);

    nn::fs::CreateDirectory(out_dir);

    nn::fs::DirectoryHandle dir_handle = {};
    nn::fs::OpenDirectory(&dir_handle, in_dir, nn::fs::OpenDirectoryMode::OpenDirectoryMode_Directory);
    s64 dir_count;
    nn::fs::GetDirectoryEntryCount(&dir_count, dir_handle);
    s64 read_count = 0;
    for (s64 i = 0; i < dir_count; ++i) {
        nn::fs::DirectoryEntry sub_dir = {};
        char sub_dir_path[nn::fs::MaxDirectoryEntryNameSize + 1];
        char sub_dir_out_path[nn::fs::MaxDirectoryEntryNameSize + 1];
        nn::fs::ReadDirectory(&read_count, &sub_dir, dir_handle, 1);
        nn::util::SNPrintf(sub_dir_path, sizeof(sub_dir_path), "%s/%s", in_dir, sub_dir.m_Name);
        nn::util::SNPrintf(sub_dir_out_path, sizeof(sub_dir_out_path), "%s/%s", out_dir, sub_dir.m_Name);
        DecompressDirectory(sub_dir_path, sub_dir_out_path, heap, file_buffer_memory, skip_mf);
    }
    nn::fs::CloseDirectory(dir_handle);
    
    read_count = 0;
    dir_handle = {};
    nn::fs::OpenDirectory(&dir_handle, in_dir, nn::fs::OpenDirectoryMode::OpenDirectoryMode_File);
    nn::fs::GetDirectoryEntryCount(&dir_count, dir_handle);
    for (s64 i = 0; i < dir_count; ++i) {
        nn::fs::DirectoryEntry file = {};
        char file_path[nn::fs::MaxDirectoryEntryNameSize + 1];
        char out_path[nn::fs::MaxDirectoryEntryNameSize + 1];
        nn::fs::ReadDirectory(&read_count, &file, dir_handle, 1);
        s32 input_size = nn::util::SNPrintf(file_path, sizeof(file_path), "%s/%s", in_dir, file.m_Name);
        nn::util::SNPrintf(out_path, sizeof(out_path), "%s/%s", out_dir, file.m_Name);

        ::memset(file_buffer_memory, 0, cDecompressedFileBufferSize);

        PRINT("Loading %s", file_path)
        nn::fs::FileHandle handle = {};
        Result res = nn::fs::OpenFile(&handle, file_path, nn::fs::OpenMode::OpenMode_Read);
        if (res != 0) {
            PRINT("Failed to load %s", file_path)
            EXL_ABORT(0x420);
        }
        s64 file_size;
        nn::fs::GetFileSize(&file_size, handle);
        nn::fs::ReadFile(handle, 0, file_buffer_memory, file_size);

        if (file_path[input_size - 6] == '.' && file_path[input_size - 5] == 'c' && file_path[input_size - 4] == 'h'
            && file_path[input_size - 3] == 'u' && file_path[input_size - 2] == 'n' && file_path[input_size - 1] == 'k') {
            DecompressFile(file_buffer_memory, file_size, heap, out_path, true);
        } else if (file_path[input_size - 5] == '.' && file_path[input_size - 4] == 'q' && file_path[input_size - 3] == 'u'
            && file_path[input_size - 2] == 'a' && file_path[input_size - 1] == 'd') {
            DecompressFile(file_buffer_memory, file_size, heap, out_path, false);
        } else {
            OutputFile(out_path, file_buffer_memory, file_size);
            PRINT("Created file %s", out_path)
        }

        nn::fs::CloseFile(handle);
    }

    nn::fs::CloseDirectory(dir_handle);
}

void DecompressAllMC(sead::Heap* heap, const char* content_dir_path, const char* output_base_path) {
        void* file_buffer_memory = heap->tryAlloc(cDecompressedFileBufferSize, 0x1000);

        if (file_buffer_memory == nullptr) {
            char buf[0x20];
            PRINT("Failed to alloc buffer")
            return;
        }

        if (g_SSBOChunkDataMgr == nullptr || g_SSBOQuadDataMgr == nullptr) {
            initializeComponents(*g_CaveModule);
        }

        DecompressDirectory(content_dir_path, output_base_path, heap, file_buffer_memory);
        char minus_field_dir[nn::fs::MaxDirectoryEntryNameSize + 1];
        char minus_field_out_dir[nn::fs::MaxDirectoryEntryNameSize + 1];
        nn::util::SNPrintf(minus_field_dir, sizeof(minus_field_dir), "%s/MinusField", content_dir_path);
        nn::util::SNPrintf(minus_field_out_dir, sizeof(minus_field_out_dir), "%s/MinusField", output_base_path);
        char buf[0x350];
        PRINT("Decompressing MinusField: %s", minus_field_dir)
        DecompressDirectory(minus_field_dir, minus_field_out_dir, heap, file_buffer_memory, false);

        heap->free(file_buffer_memory);

        return;
}


constexpr size_t cModuleSystemPostPrepareOffsetArray[] = {
    0x71008111a8,
    0x710083c2fc,
    0x7100713b74,
    0x7100802bfc,
    0x71007a3428,
    0x71007f61d0,
};
static_assert(sizeof(cModuleSystemPostPrepareOffsetArray) / sizeof(size_t) == cOffsetCount);

constexpr inline size_t cRegisterOffset[] = {
    19,
    22,
    22,
    22,
    22,
    22,
};
static_assert(sizeof(cRegisterOffset) / sizeof(size_t) == cOffsetCount);

HOOK_DEFINE_INLINE(ModuleSystemPostPrepare_v2) {
    static void Callback(exl::hook::InlineCtx* ctx) {
        g_Heap = reinterpret_cast<sead::Heap*>(ctx->X[cRegisterOffset[::GetAppVersionIndex()]]);
        return;
    }
};

HOOK_DEFINE_TRAMPOLINE(GetChunkMgr) {
    static void Callback(void* mgr, void* arg) {
        if (g_SSBOChunkDataMgr == nullptr) g_SSBOChunkDataMgr = mgr;
        Orig(mgr, arg);
    }
};
HOOK_DEFINE_TRAMPOLINE(GetQuadMgr) {
    static void Callback(void* mgr, void* arg) {
        if (g_SSBOQuadDataMgr == nullptr) g_SSBOQuadDataMgr = mgr;
        Orig(mgr, arg);
    }
};

HOOK_DEFINE_INLINE(Decompress) {
    static void Callback(exl::hook::InlineCtx* ctx) {
        DecompressAllMC(g_Heap, "content:/Cave/cave017", "sd:/Cave/cave017");
    }
};


namespace nn::oe {
    enum class FocusHandlingMode : u32 { None };
    void SetFocusHandlingMode(FocusHandlingMode);
}

HOOK_DEFINE_TRAMPOLINE(AppVersionInit) {
    static void Callback(nn::oe::FocusHandlingMode mode) {
        nn::fs::MountSdCard("sd");
        
        InitializeAppVersion();

        g_CaveModule = reinterpret_cast<void**>(exl::util::modules::GetTargetOffset(0x04698c28));
        decompressMeshCodec = reinterpret_cast<DecompFunc*>(exl::util::modules::GetTargetOffset(0x00f8c1e0));
        decompressZstd = reinterpret_cast<DecompFunc*>(exl::util::modules::GetTargetOffset(0x013864e0));
        ZSTD_readDecompressedSize = reinterpret_cast<GetSizeFunc*>(exl::util::modules::GetTargetOffset(0x00a68ea0));
        initializeComponents = reinterpret_cast<InitFunction*>(exl::util::modules::GetTargetOffset(0x00fd393c));

        const size_t offset = ::GetAppVersionOffset(cModuleSystemPostPrepareOffsetArray);
        ModuleSystemPostPrepare_v2::InstallAtOffset(offset);

        GetChunkMgr::InstallAtOffset(0x00fd60a0);
        GetQuadMgr::InstallAtOffset(0x00fd76d4);
        Decompress::InstallAtOffset(0x011880f4);

        Orig(mode);

        return;
    }
};

extern "C" void exl_main(void* x0, void* x1) {

    /* Setup hooking enviroment */
    exl::hook::Initialize();

    /* Install our application main over nnMain */
    AppVersionInit::InstallAtFuncPtr(nn::oe::SetFocusHandlingMode);

    return;
}

extern "C" NORETURN void exl_exception_entry() {
    /* TODO: exception handling */
    EXL_ABORT(0x420);
}