

#ifndef __GLOPS_DOT_H__
#define __GLOPS_DOT_H__

#include "incore.h"

extern const struct gfs2_glock_operations gfs2_meta_glops;
extern const struct gfs2_glock_operations gfs2_inode_glops;
extern const struct gfs2_glock_operations gfs2_rgrp_glops;
extern const struct gfs2_glock_operations gfs2_trans_glops;
extern const struct gfs2_glock_operations gfs2_iopen_glops;
extern const struct gfs2_glock_operations gfs2_flock_glops;
extern const struct gfs2_glock_operations gfs2_nondisk_glops;
extern const struct gfs2_glock_operations gfs2_quota_glops;
extern const struct gfs2_glock_operations gfs2_journal_glops;
extern const struct gfs2_glock_operations *gfs2_glops_list[];

#endif /* __GLOPS_DOT_H__ */
