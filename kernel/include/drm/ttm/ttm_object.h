

#ifndef _TTM_OBJECT_H_
#define _TTM_OBJECT_H_

#include <linux/list.h>
#include "drm_hashtab.h"
#include <linux/kref.h>
#include <ttm/ttm_memory.h>


enum ttm_ref_type {
	TTM_REF_USAGE,
	TTM_REF_SYNCCPU_READ,
	TTM_REF_SYNCCPU_WRITE,
	TTM_REF_NUM
};


enum ttm_object_type {
	ttm_fence_type,
	ttm_buffer_type,
	ttm_lock_type,
	ttm_driver_type0 = 256,
	ttm_driver_type1,
	ttm_driver_type2,
	ttm_driver_type3,
	ttm_driver_type4,
	ttm_driver_type5
};

struct ttm_object_file;
struct ttm_object_device;


struct ttm_base_object {
	struct drm_hash_item hash;
	enum ttm_object_type object_type;
	bool shareable;
	struct ttm_object_file *tfile;
	struct kref refcount;
	void (*refcount_release) (struct ttm_base_object **base);
	void (*ref_obj_release) (struct ttm_base_object *base,
				 enum ttm_ref_type ref_type);
};


extern int ttm_base_object_init(struct ttm_object_file *tfile,
				struct ttm_base_object *base,
				bool shareable,
				enum ttm_object_type type,
				void (*refcount_release) (struct ttm_base_object
							  **),
				void (*ref_obj_release) (struct ttm_base_object
							 *,
							 enum ttm_ref_type
							 ref_type));


extern struct ttm_base_object *ttm_base_object_lookup(struct ttm_object_file
						      *tfile, uint32_t key);


extern void ttm_base_object_unref(struct ttm_base_object **p_base);

extern int ttm_ref_object_add(struct ttm_object_file *tfile,
			      struct ttm_base_object *base,
			      enum ttm_ref_type ref_type, bool *existed);
extern int ttm_ref_object_base_unref(struct ttm_object_file *tfile,
				     unsigned long key,
				     enum ttm_ref_type ref_type);


extern struct ttm_object_file *ttm_object_file_init(struct ttm_object_device
						    *tdev,
						    unsigned int hash_order);


extern void ttm_object_file_release(struct ttm_object_file **p_tfile);


extern struct ttm_object_device *ttm_object_device_init
    (struct ttm_mem_global *mem_glob, unsigned int hash_order);


extern void ttm_object_device_release(struct ttm_object_device **p_tdev);

#endif
