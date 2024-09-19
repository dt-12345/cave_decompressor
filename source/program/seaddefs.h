#pragma once

#include "binaryoffsethelper.h"

namespace sead {

    class DirectResource {
        public:
            void *file;
            u32   file_size;
            u32   reserve0;
        public:
            virtual void Reserve0();
            virtual void Reserve1();
            virtual ~DirectResource();
    };

    class Heap {
        private:
            
        public:
            virtual ~Heap();
            virtual void Reserve0();
            virtual void Reserve1();
            virtual void Reserve2();
            virtual size_t  adjust();
            virtual void   *tryAlloc(size_t size, s32 alignment);
            virtual void    free(void *address);
            virtual size_t  freeAndGetAllocatableSize(void *address, u32 alignment);
            virtual size_t  resizeFront(void *address, size_t size);
            virtual size_t  resizeBack(void *address, size_t size);
            virtual size_t  tryRealloc(void *address, size_t new_size, s32 alignment);
            virtual size_t  freeAll();
            virtual void   *getStartAddress();
            virtual void   *getEndAddress();
            virtual size_t  getSize();
            virtual size_t  getFreeSize();
            virtual size_t  getMaxAllocatableSize(int alignment);
            virtual bool    isInclude(void *address);
            virtual bool    isEmpty();
            virtual bool    isFreeable();
            virtual bool    isResizable();
            virtual bool    isAdjustable();
    };

    class ResourceMgr {
        public:
            struct LoadArg {
                char *file_path;
                Heap *resource_heap;
                Heap *decomp_heap;
                s32   resource_alignment;
                s32   file_alignment;
                void *out_file;
                u32   out_file_size;
                s32   out_file_alignment;
                void *resource_factory;
                void *file_device;
                u32   read_div;
                u32   reserve2;
                bool *is_compressed;
            };
            static_assert(sizeof(LoadArg) == 0x50);
        public:
            using ResourceMgr0 = DirectResource * (*) (ResourceMgr *, LoadArg *, const char **, void *);
            using ResourceMgr1 = DirectResource * (*) (ResourceMgr *, LoadArg *);
        public:
            static constexpr size_t cResourceMgrSingletonOffsetArray[] = {
                0x710463bf58,
                0x7104719478,
                0x71047213a8,
                0x7104713888,
                0x7104707ce0,
                0x7104716d58,
            };
            static_assert(sizeof(cResourceMgrSingletonOffsetArray) / sizeof(size_t) == cOffsetCount);
            SINGLETON_OFFSET(ResourceMgr, cResourceMgrSingletonOffsetArray);
        public:

            static constexpr size_t cResourceMgr_tryLoadOffsetArray[] = {
                0x71008ad29c,
                0x710089be24,
                0x71007e6c7c,
                0x71008aa564,
                0x7100834748,
                0x71008b0064,
            };
            static_assert(sizeof(cResourceMgr_tryLoadOffsetArray) / sizeof(size_t) == cOffsetCount);
            DirectResource *tryLoad(LoadArg *load_arg, const char **drive, void *decompressor) {
                return FUNCTION_OFFSET(ResourceMgr0, cResourceMgr_tryLoadOffsetArray)(this, load_arg, drive, decompressor);
            }
            static constexpr size_t cResourceMgr_tryLoadWithoutDecompOffsetArray[] = {
                0,
                0,
                0,
                0,
                0,
                0x71008aff40,
            };
            static_assert(sizeof(cResourceMgr_tryLoadWithoutDecompOffsetArray) / sizeof(size_t) == cOffsetCount);
            DirectResource *tryLoadWithoutDecomp(LoadArg *load_arg) {
                return FUNCTION_OFFSET(ResourceMgr1, cResourceMgr_tryLoadWithoutDecompOffsetArray)(this, load_arg);
            }
    };
}

namespace ares {

    class ParallelMCDecompressor {
        private:
            char m_reserve[0x238];
        public:
            using ParallelMCDecompressor_ctor = void (*) (ParallelMCDecompressor*, u32, sead::Heap*, void*, size_t, size_t, u32*);
        public:

            static constexpr size_t cParallelMCDecompressor_ctorOffsetArray[] = {
                0x71011dffd4,
                0x71011379f0,
                0x7101139470,
                0x71011349e0,
                0x71011fa144,
                0x710112a7f0,
            };
            ParallelMCDecompressor(u32 priority, sead::Heap *heap, void *memory, size_t memory_size, size_t reserve0, u32 *core_mask) {
                FUNCTION_OFFSET(ParallelMCDecompressor_ctor, cParallelMCDecompressor_ctorOffsetArray)(this, priority, heap, memory, memory_size, reserve0, core_mask);
            }

            virtual ~ParallelMCDecompressor();
    };
    static_assert(sizeof(ParallelMCDecompressor) == 0x240);
}

namespace nn::util {

    class RelocationTable {
        public:
            void Unrelocate();
    };

    class BinaryFileHeader {
        public:
            bool             IsRelocated() const;
            RelocationTable *GetRelocationTable();
    };
}