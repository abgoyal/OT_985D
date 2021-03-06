
#include "tpd.h"

int TPD_RES_X = 480;
int TPD_RES_Y = 800;

//#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
int tpd_calmat[8]     = {0};
int tpd_def_calmat[8] = {0};
int tpd_calmat_size   = 8;
int tpd_def_calmat_size   = 8;
module_param_array(tpd_calmat, int, &tpd_calmat_size, 00664);
module_param_array(tpd_def_calmat, int, &tpd_def_calmat_size, 00444);
//#endif
//#ifdef TPD_TYPE_CAPACITIVE
int tpd_type_cap = 0;

module_param(tpd_type_cap, int, 00444);
int tpd_firmware_version[2] = {0,0};
int tpd_firmware_version_size = 2;
module_param_array(tpd_firmware_version, int, &tpd_firmware_version_size, 00444);

int tpd_mode = TPD_MODE_NORMAL;
int tpd_mode_axis = 0;
int tpd_mode_min = 400; //TPD_RES_Y/2;
int tpd_mode_max = 800; //TPD_RES_Y;
int tpd_mode_keypad_tolerance = 480*480/1600; //TPD_RES_X*TPD_RES_X/1600;
module_param(tpd_mode, int, 00664);
module_param(tpd_mode_axis, int, 00664);
module_param(tpd_mode_min, int, 00664);
module_param(tpd_mode_max, int, 00664);
module_param(tpd_mode_keypad_tolerance, int, 00664);

int tpd_em_debounce_time = 0;
module_param(tpd_em_debounce_time, int, 00664);

int tpd_em_spl_num = 0;
module_param(tpd_em_spl_num, int, 00664);

int tpd_em_pressure_threshold = 0;
module_param(tpd_em_pressure_threshold, int, 00664);

int tpd_load_status = 0;
module_param(tpd_load_status, int, 00664);
