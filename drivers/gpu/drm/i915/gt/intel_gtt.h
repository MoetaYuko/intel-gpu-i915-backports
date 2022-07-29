/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2020 Intel Corporation
 *
 * Please try to maintain the following order within this file unless it makes
 * sense to do otherwise. From top to bottom:
 * 1. typedefs
 * 2. #defines, and macros
 * 3. structure definitions
 * 4. function prototypes
 *
 * Within each section, please try to order by generation in ascending order,
 * from top to bottom (ie. gen6 on the top, gen8 on the bottom).
 */

#ifndef __INTEL_GTT_H__
#define __INTEL_GTT_H__

#include <linux/io-mapping.h>
#include <linux/kref.h>
#include <linux/mm.h>
#include <linux/pagevec.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>

#include <drm/drm_mm.h>

#include "gt/intel_reset.h"
#include "i915_scatterlist.h"
#include "i915_selftest.h"
#include "i915_vma_types.h"
#include "i915_params.h"
#include "intel_memory_region.h"

#define I915_GFP_ALLOW_FAIL (GFP_KERNEL | __GFP_RETRY_MAYFAIL | __GFP_NOWARN)

#if IS_ENABLED(CPTCFG_DRM_I915_TRACE_GTT)
#define DBG(...) trace_printk(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define NALLOC 3 /* 1 normal, 1 for concurrent threads, 1 for preallocation */

#define I915_GTT_PAGE_SIZE_4K	BIT_ULL(12)
#define I915_GTT_PAGE_SIZE_64K	BIT_ULL(16)
#define I915_GTT_PAGE_SIZE_2M	BIT_ULL(21)
#define I915_GTT_PAGE_SIZE_1G	BIT_ULL(30)

#define I915_GTT_PAGE_SIZE I915_GTT_PAGE_SIZE_4K
#define I915_GTT_MAX_PAGE_SIZE I915_GTT_PAGE_SIZE_1G

#define I915_GTT_PAGE_MASK -I915_GTT_PAGE_SIZE

#define I915_GTT_MIN_ALIGNMENT I915_GTT_PAGE_SIZE

#define I915_FENCE_REG_NONE -1
#define I915_MAX_NUM_FENCES 32
/* 32 fences + sign bit for FENCE_REG_NONE */
#define I915_MAX_NUM_FENCE_BITS 6

typedef u32 gen6_pte_t;
typedef u64 gen8_pte_t;

#define ggtt_total_entries(ggtt) ((ggtt)->vm.total >> PAGE_SHIFT)

#define I915_PTES(pte_len)		((unsigned int)(PAGE_SIZE / (pte_len)))
#define I915_PTE_MASK(pte_len)		(I915_PTES(pte_len) - 1)
#define I915_PDES			512
#define I915_PDE_MASK			(I915_PDES - 1)

/* gen6-hsw has bit 11-4 for physical addr bit 39-32 */
#define GEN6_GTT_ADDR_ENCODE(addr)	((addr) | (((addr) >> 28) & 0xff0))
#define GEN6_PTE_ADDR_ENCODE(addr)	GEN6_GTT_ADDR_ENCODE(addr)
#define GEN6_PDE_ADDR_ENCODE(addr)	GEN6_GTT_ADDR_ENCODE(addr)
#define GEN6_PTE_CACHE_LLC		(2 << 1)
#define GEN6_PTE_UNCACHED		(1 << 1)
#define GEN6_PTE_VALID			REG_BIT(0)

#define GEN6_PTES			I915_PTES(sizeof(gen6_pte_t))
#define GEN6_PD_SIZE			(I915_PDES * PAGE_SIZE)
#define GEN6_PD_ALIGN			(PAGE_SIZE * 16)
#define GEN6_PDE_SHIFT			22
#define GEN6_PDE_VALID			REG_BIT(0)
#define NUM_PTE(pde_shift)     (1 << (pde_shift - PAGE_SHIFT))

#define GEN7_PTE_CACHE_L3_LLC		(3 << 1)

#define BYT_PTE_SNOOPED_BY_CPU_CACHES	REG_BIT(2)
#define BYT_PTE_WRITEABLE		REG_BIT(1)

#define GEN12_PPGTT_PTE_LM	BIT_ULL(11)
#define GEN12_USM_PPGTT_PTE_AE	BIT_ULL(10)
#define GEN12_PPGTT_PTE_PAT2	BIT_ULL(7)
#define GEN12_PPGTT_PTE_NC	BIT_ULL(5)
#define GEN12_PPGTT_PTE_PAT1	BIT_ULL(4)
#define GEN12_PPGTT_PTE_PAT0	BIT_ULL(3)

/*
 *  DOC: GEN12 GGTT Table Entry format
 *
 * TGL:
 *
 * +----------+---------+---------+-----------------+--------------+---------+
 * |	63:46 |   45:12 |    11:5 |		4:2 |		 1 |	   0 |
 * +==========+=========+=========+=================+==============+=========+
 * |  Ignored | Address | Ignored | Function Number | Local Memory | Present |
 * +----------+---------+---------+-----------------+--------------+---------+
 *
 * ADL-P/S:
 * +----------+--------------+-------------------+---------+---------+----------+--------+---------+
 * |	63:46 |        45:42 |		   41:39 |   38:12 |   11:5  |	    4:2 |      1 |	 0 |
 * +==========+==============+===================+=========+=========+==========+========+=========+
 * |  Ignored | MKTME key ID | 2LM Far Memory	 | Address | Ignored | Function | Local  | Present |
 * |	      |		     | address extension |	   |	     | Number	| Memory |	   |
 * +----------+--------------+-------------------+---------+---------+----------+--------+---------+
 *
 * Platforms supporting more than 7 VFs (XEHPSDV and later):
 *
 * +----------+---------+-----------------+--------------+---------+
 * |	63:46 |   45:12 |	     11:2 |	       1 |	 0 |
 * +==========+=========+=================+==============+=========+
 * |  Ignored | Address | Function Number | Local Memory | Present |
 * +----------+---------+-----------------+--------------+---------+
 */

#define GEN12_GGTT_PTE_LM		BIT_ULL(1)
#define TGL_GGTT_PTE_VFID_MASK		GENMASK_ULL(4, 2)
#define XEHPSDV_GGTT_PTE_VFID_MASK	GENMASK_ULL(11, 2)
#define GEN12_GGTT_PTE_ADDR_MASK	GENMASK_ULL(45, 12)
#define ADL_GGTT_PTE_ADDR_MASK		GENMASK_ULL(38, 12)

#define GEN12_PDE_64K BIT(6)
#define GEN12_PTE_PS64 BIT(8)

/*
 * Cacheability Control is a 4-bit value. The low three bits are stored in bits
 * 3:1 of the PTE, while the fourth bit is stored in bit 11 of the PTE.
 */
#define HSW_CACHEABILITY_CONTROL(bits)	((((bits) & 0x7) << 1) | \
					 (((bits) & 0x8) << (11 - 3)))
#define HSW_WB_LLC_AGE3			HSW_CACHEABILITY_CONTROL(0x2)
#define HSW_WB_LLC_AGE0			HSW_CACHEABILITY_CONTROL(0x3)
#define HSW_WB_ELLC_LLC_AGE3		HSW_CACHEABILITY_CONTROL(0x8)
#define HSW_WB_ELLC_LLC_AGE0		HSW_CACHEABILITY_CONTROL(0xb)
#define HSW_WT_ELLC_LLC_AGE3		HSW_CACHEABILITY_CONTROL(0x7)
#define HSW_WT_ELLC_LLC_AGE0		HSW_CACHEABILITY_CONTROL(0x6)
#define HSW_PTE_UNCACHED		(0)
#define HSW_GTT_ADDR_ENCODE(addr)	((addr) | (((addr) >> 28) & 0x7f0))
#define HSW_PTE_ADDR_ENCODE(addr)	HSW_GTT_ADDR_ENCODE(addr)

/*
 * GEN8 32b style address is defined as a 3 level page table:
 * 31:30 | 29:21 | 20:12 |  11:0
 * PDPE  |  PDE  |  PTE  | offset
 * The difference as compared to normal x86 3 level page table is the PDPEs are
 * programmed via register.
 *
 * GEN8 48b style address is defined as a 4 level page table:
 * 47:39 | 38:30 | 29:21 | 20:12 |  11:0
 * PML4E | PDPE  |  PDE  |  PTE  | offset
 */
#define GEN8_3LVL_PDPES			4

#define PPAT_UNCACHED			(_PAGE_PWT | _PAGE_PCD)
#define PPAT_CACHED_PDE			0 /* WB LLC */
#define PPAT_CACHED			_PAGE_PAT /* WB LLCeLLC */
#define PPAT_DISPLAY_ELLC		_PAGE_PCD /* WT eLLC */

#define CHV_PPAT_SNOOP			REG_BIT(6)
#define GEN12_PPAT_CLOS(x)		((x)<<2)
#define GEN8_PPAT_AGE(x)		((x)<<4)
#define GEN8_PPAT_LLCeLLC		(3<<2)
#define GEN8_PPAT_LLCELLC		(2<<2)
#define GEN8_PPAT_LLC			(1<<2)
#define GEN8_PPAT_WB			(3<<0)
#define GEN8_PPAT_WT			(2<<0)
#define GEN8_PPAT_WC			(1<<0)
#define GEN8_PPAT_UC			(0<<0)
#define GEN8_PPAT_ELLC_OVERRIDE		(0<<2)
#define GEN8_PPAT(i, x)			((u64)(x) << ((i) * 8))

#define GEN8_PAGE_PRESENT		BIT_ULL(0)
#define GEN8_PAGE_RW			BIT_ULL(1)

#define GEN8_PDE_IPS_64K BIT_ULL(11)
#define GEN8_PDE_PS_2M	 BIT_ULL(7)
#define GEN8_PDPE_PS_1G  BIT_ULL(7)

enum i915_cache_level;

struct drm_i915_gem_object;
struct i915_drm_client;
struct i915_fence_reg;
struct i915_vma;
struct intel_gt;

#define for_each_sgt_daddr(__dp, __iter, __sgt) \
	__for_each_sgt_daddr(__dp, __iter, __sgt, I915_GTT_PAGE_SIZE)

/* Same as for_each_sgt_daddr but iterator is pre-initialized */
#define for_each_daddr(__dp, __iter) \
	__for_each_daddr(__dp, __iter, I915_GTT_PAGE_SIZE)

struct i915_page_table {
	struct drm_i915_gem_object *base;
	union {
		atomic_t used;
		struct i915_page_table *stash;
	};
	bool is_compact;
};

struct i915_page_directory {
	struct i915_page_table pt;
	spinlock_t lock;
	void **entry;
};

#define __px_choose_expr(x, type, expr, other) \
	__builtin_choose_expr( \
	__builtin_types_compatible_p(typeof(x), type) || \
	__builtin_types_compatible_p(typeof(x), const type), \
	({ type __x = (type)(x); expr; }), \
	other)

#define px_base(px) \
	__px_choose_expr(px, struct drm_i915_gem_object *, __x, \
	__px_choose_expr(px, struct i915_page_table *, __x->base, \
	__px_choose_expr(px, struct i915_page_directory *, __x->pt.base, \
	(void)0)))

struct page *__px_page(struct drm_i915_gem_object *p);
dma_addr_t __px_dma(struct drm_i915_gem_object *p);
#define px_dma(px) (__px_dma(px_base(px)))

void *__px_vaddr(struct drm_i915_gem_object *p, bool *needs_flush);
#define px_vaddr(px, needs_flush) (__px_vaddr(px_base(px), needs_flush))

#define px_pt(px) \
	__px_choose_expr(px, struct i915_page_table *, __x, \
	__px_choose_expr(px, struct i915_page_directory *, &__x->pt, \
	(void)0))
#define px_used(px) (&px_pt(px)->used)

struct i915_vm_pt_stash {
	/* preallocated chains of page tables/directories */
	struct i915_page_table *pt[2];
};

struct i915_vma_ops {
	/* Map an object into an address space with the given cache flags. */
	void (*bind_vma)(struct i915_address_space *vm,
			 struct i915_vm_pt_stash *stash,
			 struct i915_vma *vma,
			 enum i915_cache_level cache_level,
			 u32 flags);
	/*
	 * Unmap an object from an address space. This usually consists of
	 * setting the valid PTE entries to a reserved scratch page.
	 */
	void (*unbind_vma)(struct i915_address_space *vm,
			   struct i915_vma *vma);

	int (*set_pages)(struct i915_vma *vma);
	void (*clear_pages)(struct i915_vma *vma);
};

struct i915_svm;

struct i915_address_space {
	struct kref ref;
	struct rcu_work rcu;

	struct drm_mm mm;
	struct intel_gt *gt;
	struct drm_i915_private *i915;
	struct device *dma;

	struct inode *inode;

	u32 asid;
	u32 poison; /* value used to fill the scratch page */

	/*
	 * Every address space belongs to a struct file, a single client -
	 * except for the global GTT that is owned by the driver (and so @file
	 * is set to NULL). In principle, no information should leak from one
	 * context to another (or between files/processes etc) unless
	 * explicitly shared by the owner. Tracking the owner is important in
	 * order to free up per-file objects along with the file, to aide
	 * resource tracking, and to assign blame.
	 */
	struct i915_drm_client *client;

	u64 total;		/* size addr space maps (ex. 2GB for ggtt) */
	u64 reserved;		/* size addr space reserved */
	u64 min_alignment[INTEL_MEMORY_STOLEN_LOCAL + 1];

	unsigned int bind_async_flags;

	/*
	 * Each active user context has its own address space (in full-ppgtt).
	 * Since the vm may be shared between multiple contexts, we count how
	 * many contexts keep us "open". Once open hits zero, we are closed
	 * and do not allow any new attachments, and proceed to shutdown our
	 * vma and page directories.
	 */
	atomic_t open;
	struct work_struct close_work;

	struct mutex mutex; /* protects vma and our lists */

	struct kref resv_ref; /* kref to keep the reservation lock alive. */
	struct dma_resv _resv; /* reservation lock for all pd objects, and buffer pool */
#define VM_CLASS_GGTT 0
#define VM_CLASS_PPGTT 1
#define VM_CLASS_DPT 2

	struct drm_i915_gem_object *scratch[5];

	/**
	 * List of vma currently bound.
	 */
	struct list_head bound_list;

	/**
	 * List of VM_BIND objects.
	 */
	struct mutex vm_bind_lock;  /* Protects vm_bind lists */
	struct list_head vm_bind_list;
	struct list_head vm_bound_list;
	struct list_head vm_capture_list;
	struct list_head vm_rebind_list;
	spinlock_t vm_capture_lock;  /* Protects vm_capture_list */
	spinlock_t vm_rebind_lock;   /* Protects vm_rebind_list */
	bool invalidate_tlb_scratch:1;
	/* va tree of persistent vmas */
	struct rb_root_cached va;
	struct list_head non_priv_vm_bind_list;
	struct drm_i915_gem_object *root_obj;
	struct list_head priv_obj_list;

	struct {
		struct i915_vma *vma;
		struct drm_i915_gem_object *obj;
	} mfence;

	/* EU debugger */
	spinlock_t debugger_lock;
	struct list_head debugger_fence_list;

	/* SVM */
	struct i915_svm *svm;
	struct mutex svm_mutex; /* protects svm enabling */

	unsigned long flags;
#define I915_VM_HAS_PERSISTENT_BINDS 0

	/* Global GTT */
	bool is_ggtt:1;

	/* Display page table */
	bool is_dpt:1;

	/* Some systems support read-only mappings for GGTT and/or PPGTT */
	bool has_read_only:1;

	/**
	 * true: allow only vm_bind method of binding.
	 * false: allow only legacy execbuff method of binding.
	 */
	bool vm_bind_mode:1;

	/* Does address space maps to a valid scratch page */
	bool has_scratch:1;

	/* Is address space enabled for recoverable page faults? */
	bool page_fault_enabled:1;

	u8 top;
	u8 pd_shift;
	u8 scratch_order;

	struct drm_i915_gem_object *
		(*alloc_pt_dma)(struct i915_address_space *vm, int sz);
	struct drm_i915_gem_object *
		(*alloc_scratch_dma)(struct i915_address_space *vm, int sz);

	u64 (*pte_encode)(dma_addr_t addr,
			  enum i915_cache_level level,
			  u32 flags); /* Create a valid PTE */
#define PTE_READ_ONLY	BIT(0)
#define PTE_LM		BIT(1)
#define PTE_AE		BIT(2)

	void (*allocate_va_range)(struct i915_address_space *vm,
				  struct i915_vm_pt_stash *stash,
				  u64 start, u64 length);
	void (*clear_range)(struct i915_address_space *vm,
			    u64 start, u64 length);
	void (*dump_va_range)(struct i915_address_space *vm,
			      u64 start, u64 length);
	void (*insert_page)(struct i915_address_space *vm,
			    dma_addr_t addr,
			    u64 offset,
			    enum i915_cache_level cache_level,
			    u32 flags);
	void (*insert_entries)(struct i915_address_space *vm,
			       struct i915_vma *vma,
			       enum i915_cache_level cache_level,
			       u32 flags);
	void (*cleanup)(struct i915_address_space *vm);

	struct i915_vma_ops vma_ops;

	I915_SELFTEST_DECLARE(struct fault_attr fault_attr);
	I915_SELFTEST_DECLARE(bool scrub_64K);

	struct i915_active active;

	/**
	 * List of persistent vmas which are pending destruction.
	 */
	struct llist_head vm_bind_free_list;
	struct work_struct vm_bind_free_work;

	/* Per tile active users of this VM */
	atomic_t active_contexts_gt[I915_MAX_GT];
};

/*
 * The Graphics Translation Table is the way in which GEN hardware translates a
 * Graphics Virtual Address into a Physical Address. In addition to the normal
 * collateral associated with any va->pa translations GEN hardware also has a
 * portion of the GTT which can be mapped by the CPU and remain both coherent
 * and correct (in cases like swizzling). That region is referred to as GMADR in
 * the spec.
 */
struct i915_ggtt {
	struct i915_address_space vm;

	struct io_mapping iomap;	/* Mapping to our CPU mappable region */
	struct resource gmadr;		/* GMADR resource */
	resource_size_t mappable_end;	/* End offset that we can CPU map */

	/** "Graphics Stolen Memory" holds the global PTEs */
	void __iomem *gsm;
	void (*invalidate)(struct i915_ggtt *ggtt);

	/** PPGTT used for aliasing the PPGTT with the GTT */
	struct i915_ppgtt *alias;

	bool do_idle_maps;

	int mtrr;

	/** Bit 6 swizzling required for X tiling */
	u32 bit_6_swizzle_x;
	/** Bit 6 swizzling required for Y tiling */
	u32 bit_6_swizzle_y;

	u32 pin_bias;

	unsigned int num_fences;
	struct i915_fence_reg *fence_regs;
	struct list_head fence_list;

	/**
	 * List of all objects in gtt_space, currently mmaped by userspace.
	 * All objects within this list must also be on bound_list.
	 */
	struct list_head userfault_list;

	/* Manual runtime pm autosuspend delay for user GGTT mmaps */
	struct intel_wakeref_auto userfault_wakeref;

	struct mutex error_mutex;
	struct drm_mm_node error_capture;
	struct drm_mm_node uc_fw;
};

struct i915_ppgtt {
	struct i915_address_space vm;

	struct i915_page_directory *pd;
};

#define i915_is_ggtt(vm) ((vm)->is_ggtt)
#define i915_is_dpt(vm) ((vm)->is_dpt)
#define i915_is_ggtt_or_dpt(vm) (i915_is_ggtt(vm) || i915_is_dpt(vm))

bool intel_vm_no_concurrent_access_wa(struct drm_i915_private *i915);

int __must_check
i915_vm_lock_objects(struct i915_address_space *vm, struct i915_gem_ww_ctx *ww);

static inline unsigned int
i915_vm_lvl(const struct i915_address_space * const vm)
{
	return vm->top + 1;
}

static inline bool
i915_vm_has_scratch_64K(struct i915_address_space *vm)
{
	return vm->scratch_order == get_order(I915_GTT_PAGE_SIZE_64K);
}

static inline u64 i915_vm_min_alignment(struct i915_address_space *vm,
					enum intel_memory_type type)
{
	return vm->min_alignment[type];
}

static inline bool
i915_vm_has_cache_coloring(struct i915_address_space *vm)
{
	return i915_is_ggtt(vm) && vm->mm.color_adjust;
}

static inline bool
i915_vm_has_memory_coloring(struct i915_address_space *vm)
{
       return !i915_is_ggtt(vm) && vm->mm.color_adjust;
}

static inline bool
i915_vm_page_fault_enabled(struct i915_address_space *vm)
{
	return vm->page_fault_enabled;
}

static inline struct i915_ggtt *
i915_vm_to_ggtt(struct i915_address_space *vm)
{
	BUILD_BUG_ON(offsetof(struct i915_ggtt, vm));
	GEM_BUG_ON(!i915_is_ggtt(vm));
	return container_of(vm, struct i915_ggtt, vm);
}

static inline struct i915_ppgtt *
i915_vm_to_ppgtt(struct i915_address_space *vm)
{
	BUILD_BUG_ON(offsetof(struct i915_ppgtt, vm));
	GEM_BUG_ON(i915_is_ggtt_or_dpt(vm));
	return container_of(vm, struct i915_ppgtt, vm);
}

static inline struct i915_address_space *
i915_vm_get(struct i915_address_space *vm)
{
	kref_get(&vm->ref);
	return vm;
}

/**
 * i915_vm_resv_get - Obtain a reference on the vm's reservation lock
 * @vm: The vm whose reservation lock we want to share.
 *
 * Return: A pointer to the vm's reservation lock.
 */
static inline struct dma_resv *i915_vm_resv_get(struct i915_address_space *vm)
{
	kref_get(&vm->resv_ref);
	return &vm->_resv;
}

static inline struct i915_address_space *
i915_vm_tryget(struct i915_address_space *vm)
{
	if (likely(kref_get_unless_zero(&vm->ref)))
		return vm;

	return NULL;
}

void i915_vm_release(struct kref *kref);

void i915_vm_resv_release(struct kref *kref);

static inline void i915_vm_put(struct i915_address_space *vm)
{
	kref_put(&vm->ref, i915_vm_release);
}

/**
 * i915_vm_resv_put - Release a reference on the vm's reservation lock
 * @resv: Pointer to a reservation lock obtained from i915_vm_resv_get()
 */
static inline void i915_vm_resv_put(struct i915_address_space *vm)
{
	kref_put(&vm->resv_ref, i915_vm_resv_release);
}

static inline struct i915_address_space *
i915_vm_open(struct i915_address_space *vm)
{
	GEM_BUG_ON(!atomic_read(&vm->open));
	atomic_inc(&vm->open);
	return i915_vm_get(vm);
}

static inline bool
i915_vm_tryopen(struct i915_address_space *vm)
{
	if (atomic_add_unless(&vm->open, 1, 0))
		return i915_vm_get(vm);

	return false;
}

void i915_vm_close(struct i915_address_space *vm);

int i915_address_space_init(struct i915_address_space *vm, int subclass);
void i915_address_space_fini(struct i915_address_space *vm);

static inline u32 i915_pte_index(u64 address, unsigned int pde_shift)
{
	const u32 mask = NUM_PTE(pde_shift) - 1;

	return (address >> PAGE_SHIFT) & mask;
}

/*
 * Helper to counts the number of PTEs within the given length. This count
 * does not cross a page table boundary, so the max value would be
 * GEN6_PTES for GEN6, and GEN8_PTES for GEN8.
 */
static inline u32 i915_pte_count(u64 addr, u64 length, unsigned int pde_shift)
{
	const u64 mask = ~((1ULL << pde_shift) - 1);
	u64 end;

	GEM_BUG_ON(length == 0);
	GEM_BUG_ON(offset_in_page(addr | length));

	end = addr + length;

	if ((addr & mask) != (end & mask))
		return NUM_PTE(pde_shift) - i915_pte_index(addr, pde_shift);

	return i915_pte_index(end, pde_shift) - i915_pte_index(addr, pde_shift);
}

static inline u32 i915_pde_index(u64 addr, u32 shift)
{
	return (addr >> shift) & I915_PDE_MASK;
}

static inline struct i915_page_table *
i915_pt_entry(const struct i915_page_directory * const pd,
	      const unsigned short n)
{
	return pd->entry[n];
}

static inline struct i915_page_directory *
i915_pd_entry(const struct i915_page_directory * const pdp,
	      const unsigned short n)
{
	return pdp->entry[n];
}

static inline dma_addr_t
i915_page_dir_dma_addr(const struct i915_ppgtt *ppgtt, const unsigned int n)
{
	struct i915_page_table *pt = ppgtt->pd->entry[n];

	return __px_dma(pt ? px_base(pt) : ppgtt->vm.scratch[ppgtt->vm.top]);
}

int ppgtt_init(struct i915_ppgtt *ppgtt, struct intel_gt *gt);

int i915_ggtt_probe_hw(struct drm_i915_private *i915);
int i915_ggtt_init_hw(struct drm_i915_private *i915);
int i915_ggtt_enable_hw(struct drm_i915_private *i915);
void i915_ggtt_enable_guc(struct i915_ggtt *ggtt);
void i915_ggtt_disable_guc(struct i915_ggtt *ggtt);
int i915_init_ggtt(struct drm_i915_private *i915);
void i915_ggtt_driver_release(struct drm_i915_private *i915);
void i915_ggtt_driver_late_release(struct drm_i915_private *i915);

static inline bool i915_ggtt_has_aperture(const struct i915_ggtt *ggtt)
{
	return ggtt->mappable_end > 0;
}

void intel_partial_pages_for_sg_table(struct drm_i915_gem_object *obj,
				      struct sg_table *st,
				      unsigned long obj_offset,
				      unsigned long page_count,
				      struct scatterlist **sgl);
int i915_ggtt_balloon(struct i915_ggtt *ggtt, u64 start, u64 end,
		      struct drm_mm_node *node);
void i915_ggtt_deballoon(struct i915_ggtt *ggtt, struct drm_mm_node *node);

inline bool i915_ggtt_has_xehpsdv_pte_vfid_mask(struct i915_ggtt *ggtt);

void i915_ggtt_set_space_owner(struct i915_ggtt *ggtt, u16 vfid,
			       const struct drm_mm_node *node);

#define I915_GGTT_SAVE_PTES_NO_VFID BIT(31)

int i915_ggtt_save_ptes(struct i915_ggtt *ggtt, const struct drm_mm_node *node, void *buf,
			unsigned int size, unsigned int flags);

#define I915_GGTT_RESTORE_PTES_NEW_VFID  BIT(31)
#define I915_GGTT_RESTORE_PTES_VFID_MASK GENMASK(19, 0)

int i915_ggtt_restore_ptes(struct i915_ggtt *ggtt, const struct drm_mm_node *node, const void *buf,
			   unsigned int size, unsigned int flags);

int i915_ppgtt_init_hw(struct intel_gt *gt);

struct i915_ppgtt *i915_ppgtt_create(struct intel_gt *gt, u32 flags);

void i915_ggtt_suspend_vm(struct i915_address_space *vm);
bool i915_ggtt_resume_vm(struct i915_address_space *vm);
void i915_ggtt_suspend(struct i915_ggtt *gtt);
void i915_ggtt_resume(struct i915_ggtt *ggtt);

void
fill_page_dma(struct drm_i915_gem_object *p, const u64 val, unsigned int count);

#define fill_px(px, v) fill_page_dma(px_base(px), (v), PAGE_SIZE / sizeof(u64))
#define fill32_px(px, v) do {						\
	u64 v__ = lower_32_bits(v);					\
	fill_px((px), v__ << 32 | v__);					\
} while (0)

int setup_scratch_page(struct i915_address_space *vm);
void free_scratch(struct i915_address_space *vm);

struct drm_i915_gem_object *alloc_pt_dma(struct i915_address_space *vm, int sz);
struct drm_i915_gem_object *alloc_pt_lmem(struct i915_address_space *vm, int sz);
struct i915_page_table *alloc_pt(struct i915_address_space *vm);
struct i915_page_directory *alloc_pd(struct i915_address_space *vm);
struct i915_page_directory *__alloc_pd(int npde);

int map_pt_dma(struct i915_address_space *vm, struct drm_i915_gem_object *obj);
int map_pt_dma_locked(struct i915_address_space *vm, struct drm_i915_gem_object *obj);

void free_px(struct i915_address_space *vm,
	     struct i915_page_table *pt, int lvl);
#define free_pt(vm, px) free_px(vm, px, 0)
#define free_pd(vm, px) free_px(vm, px_pt(px), 1)

void
__set_pd_entry(struct i915_page_directory * const pd,
	       const unsigned short idx,
	       struct i915_page_table *pt,
	       u64 (*encode)(const dma_addr_t, const enum i915_cache_level));

#define set_pd_entry(pd, idx, to) \
	__set_pd_entry((pd), (idx), px_pt(to), gen8_pde_encode)

void
clear_pd_entry(struct i915_page_directory * const pd,
	       const unsigned short idx,
	       const struct drm_i915_gem_object * const scratch);

bool
release_pd_entry(struct i915_page_directory * const pd,
		 const unsigned short idx,
		 struct i915_page_table * const pt,
		 const struct drm_i915_gem_object * const scratch);
void gen6_ggtt_invalidate(struct i915_ggtt *ggtt);

void gen8_set_pte(void __iomem *addr, gen8_pte_t pte);
gen8_pte_t gen8_get_pte(void __iomem *addr);

u64 ggtt_addr_to_pte_offset(u64 ggtt_addr);

void __gen8_ggtt_insert_page_wa_bcs(struct i915_ggtt *ggtt, u32 vfid,
				    dma_addr_t addr, u64 offset,
				    enum i915_cache_level level, u32 flags);

int ggtt_set_pages(struct i915_vma *vma);
int ppgtt_set_pages(struct i915_vma *vma);
void clear_pages(struct i915_vma *vma);

void ppgtt_bind_vma(struct i915_address_space *vm,
		    struct i915_vm_pt_stash *stash,
		    struct i915_vma *vma,
		    enum i915_cache_level cache_level,
		    u32 flags);
void ppgtt_unbind_vma(struct i915_address_space *vm,
		      struct i915_vma *vma);

void gtt_write_workarounds(struct intel_gt *gt);

void setup_private_pat(struct intel_uncore *uncore);

int i915_vm_alloc_pt_stash(struct i915_address_space *vm,
			   struct i915_vm_pt_stash *stash,
			   u64 size);
int i915_vm_map_pt_stash(struct i915_address_space *vm,
			 struct i915_vm_pt_stash *stash);
void i915_vm_free_pt_stash(struct i915_address_space *vm,
			   struct i915_vm_pt_stash *stash);

struct i915_vma *
__vm_create_scratch_for_read(struct i915_address_space *vm, unsigned long size);

struct i915_vma *
__vm_create_scratch_for_read_pinned(struct i915_address_space *vm, unsigned long size);

static inline struct sgt_dma {
	struct scatterlist *sg;
	dma_addr_t dma, max;
	u64 rem;
} sgt_dma(struct i915_vma *vma) {
	struct scatterlist *sg = vma->pages->sgl;
	u64 max, offset = 0;
	dma_addr_t addr;

	/* For partial binding, skip until specified offset */
	if (vma->ggtt_view.type == I915_GGTT_VIEW_PARTIAL) {
		offset = vma->ggtt_view.partial.offset << PAGE_SHIFT;
		while (offset >= sg_dma_len(sg)) {
			offset -= sg_dma_len(sg);
			sg = __sg_next(sg);
		}
	}

	addr = sg_dma_address(sg) + offset;
	max = addr + min_t(u64, (sg_dma_len(sg) - offset), vma->size);
	return (struct sgt_dma) { sg, addr, max, vma->size };
}

#ifdef CPTCFG_DRM_I915_DUMP_PPGTT
static inline void ppgtt_dump(struct i915_address_space *vm,
			      u64 start, u64 length)
{
	if (vm->dump_va_range)
		vm->dump_va_range(vm, start, length);
}
#else
static inline void ppgtt_dump(struct i915_address_space *vm,
			      u64 start, u64 length) { }
#endif

/* SVM UAPI */
#define I915_GTT_SVM_READONLY  BIT(0)
#define I915_GTT_SVM_LMEM      BIT(1)

int svm_bind_addr_prepare(struct i915_address_space *vm,
			  struct i915_vm_pt_stash *stash,
			  struct i915_gem_ww_ctx *ww,
			  u64 start, u64 size);
int svm_bind_addr_commit(struct i915_address_space *vm, u64 start, u64 size,
			 u64 flags, struct sg_table *st, u32 sg_page_sizes);
int svm_bind_addr(struct i915_address_space *vm, struct i915_gem_ww_ctx *ctx,
		  u64 start, u64 size, u64 flags,
		  struct sg_table *st, u32 sg_page_sizes);
void svm_unbind_addr(struct i915_address_space *vm, u64 start, u64 size);

#endif