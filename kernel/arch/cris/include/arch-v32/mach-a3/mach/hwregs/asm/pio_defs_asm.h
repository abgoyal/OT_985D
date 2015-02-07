
#ifndef __pio_defs_asm_h
#define __pio_defs_asm_h


#ifndef REG_FIELD
#define REG_FIELD( scope, reg, field, value ) \
  REG_FIELD_X_( value, reg_##scope##_##reg##___##field##___lsb )
#define REG_FIELD_X_( value, shift ) ((value) << shift)
#endif

#ifndef REG_STATE
#define REG_STATE( scope, reg, field, symbolic_value ) \
  REG_STATE_X_( regk_##scope##_##symbolic_value, reg_##scope##_##reg##___##field##___lsb )
#define REG_STATE_X_( k, shift ) (k << shift)
#endif

#ifndef REG_MASK
#define REG_MASK( scope, reg, field ) \
  REG_MASK_X_( reg_##scope##_##reg##___##field##___width, reg_##scope##_##reg##___##field##___lsb )
#define REG_MASK_X_( width, lsb ) (((1 << width)-1) << lsb)
#endif

#ifndef REG_LSB
#define REG_LSB( scope, reg, field ) reg_##scope##_##reg##___##field##___lsb
#endif

#ifndef REG_BIT
#define REG_BIT( scope, reg, field ) reg_##scope##_##reg##___##field##___bit
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) REG_ADDR_X_(inst, reg_##scope##_##reg##_offset)
#define REG_ADDR_X_( inst, offs ) ((inst) + offs)
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
         REG_ADDR_VECT_X_(inst, reg_##scope##_##reg##_offset, index, \
			 STRIDE_##scope##_##reg )
#define REG_ADDR_VECT_X_( inst, offs, index, stride ) \
                          ((inst) + offs + (index) * stride)
#endif

/* Register rw_data, scope pio, type rw */
#define reg_pio_rw_data_offset 64

/* Register rw_io_access0, scope pio, type rw */
#define reg_pio_rw_io_access0___data___lsb 0
#define reg_pio_rw_io_access0___data___width 8
#define reg_pio_rw_io_access0_offset 0

/* Register rw_io_access1, scope pio, type rw */
#define reg_pio_rw_io_access1___data___lsb 0
#define reg_pio_rw_io_access1___data___width 8
#define reg_pio_rw_io_access1_offset 4

/* Register rw_io_access2, scope pio, type rw */
#define reg_pio_rw_io_access2___data___lsb 0
#define reg_pio_rw_io_access2___data___width 8
#define reg_pio_rw_io_access2_offset 8

/* Register rw_io_access3, scope pio, type rw */
#define reg_pio_rw_io_access3___data___lsb 0
#define reg_pio_rw_io_access3___data___width 8
#define reg_pio_rw_io_access3_offset 12

/* Register rw_io_access4, scope pio, type rw */
#define reg_pio_rw_io_access4___data___lsb 0
#define reg_pio_rw_io_access4___data___width 8
#define reg_pio_rw_io_access4_offset 16

/* Register rw_io_access5, scope pio, type rw */
#define reg_pio_rw_io_access5___data___lsb 0
#define reg_pio_rw_io_access5___data___width 8
#define reg_pio_rw_io_access5_offset 20

/* Register rw_io_access6, scope pio, type rw */
#define reg_pio_rw_io_access6___data___lsb 0
#define reg_pio_rw_io_access6___data___width 8
#define reg_pio_rw_io_access6_offset 24

/* Register rw_io_access7, scope pio, type rw */
#define reg_pio_rw_io_access7___data___lsb 0
#define reg_pio_rw_io_access7___data___width 8
#define reg_pio_rw_io_access7_offset 28

/* Register rw_io_access8, scope pio, type rw */
#define reg_pio_rw_io_access8___data___lsb 0
#define reg_pio_rw_io_access8___data___width 8
#define reg_pio_rw_io_access8_offset 32

/* Register rw_io_access9, scope pio, type rw */
#define reg_pio_rw_io_access9___data___lsb 0
#define reg_pio_rw_io_access9___data___width 8
#define reg_pio_rw_io_access9_offset 36

/* Register rw_io_access10, scope pio, type rw */
#define reg_pio_rw_io_access10___data___lsb 0
#define reg_pio_rw_io_access10___data___width 8
#define reg_pio_rw_io_access10_offset 40

/* Register rw_io_access11, scope pio, type rw */
#define reg_pio_rw_io_access11___data___lsb 0
#define reg_pio_rw_io_access11___data___width 8
#define reg_pio_rw_io_access11_offset 44

/* Register rw_io_access12, scope pio, type rw */
#define reg_pio_rw_io_access12___data___lsb 0
#define reg_pio_rw_io_access12___data___width 8
#define reg_pio_rw_io_access12_offset 48

/* Register rw_io_access13, scope pio, type rw */
#define reg_pio_rw_io_access13___data___lsb 0
#define reg_pio_rw_io_access13___data___width 8
#define reg_pio_rw_io_access13_offset 52

/* Register rw_io_access14, scope pio, type rw */
#define reg_pio_rw_io_access14___data___lsb 0
#define reg_pio_rw_io_access14___data___width 8
#define reg_pio_rw_io_access14_offset 56

/* Register rw_io_access15, scope pio, type rw */
#define reg_pio_rw_io_access15___data___lsb 0
#define reg_pio_rw_io_access15___data___width 8
#define reg_pio_rw_io_access15_offset 60

/* Register rw_ce0_cfg, scope pio, type rw */
#define reg_pio_rw_ce0_cfg___lw___lsb 0
#define reg_pio_rw_ce0_cfg___lw___width 6
#define reg_pio_rw_ce0_cfg___ew___lsb 6
#define reg_pio_rw_ce0_cfg___ew___width 3
#define reg_pio_rw_ce0_cfg___zw___lsb 9
#define reg_pio_rw_ce0_cfg___zw___width 3
#define reg_pio_rw_ce0_cfg___aw___lsb 12
#define reg_pio_rw_ce0_cfg___aw___width 2
#define reg_pio_rw_ce0_cfg___mode___lsb 14
#define reg_pio_rw_ce0_cfg___mode___width 2
#define reg_pio_rw_ce0_cfg_offset 68

/* Register rw_ce1_cfg, scope pio, type rw */
#define reg_pio_rw_ce1_cfg___lw___lsb 0
#define reg_pio_rw_ce1_cfg___lw___width 6
#define reg_pio_rw_ce1_cfg___ew___lsb 6
#define reg_pio_rw_ce1_cfg___ew___width 3
#define reg_pio_rw_ce1_cfg___zw___lsb 9
#define reg_pio_rw_ce1_cfg___zw___width 3
#define reg_pio_rw_ce1_cfg___aw___lsb 12
#define reg_pio_rw_ce1_cfg___aw___width 2
#define reg_pio_rw_ce1_cfg___mode___lsb 14
#define reg_pio_rw_ce1_cfg___mode___width 2
#define reg_pio_rw_ce1_cfg_offset 72

/* Register rw_ce2_cfg, scope pio, type rw */
#define reg_pio_rw_ce2_cfg___lw___lsb 0
#define reg_pio_rw_ce2_cfg___lw___width 6
#define reg_pio_rw_ce2_cfg___ew___lsb 6
#define reg_pio_rw_ce2_cfg___ew___width 3
#define reg_pio_rw_ce2_cfg___zw___lsb 9
#define reg_pio_rw_ce2_cfg___zw___width 3
#define reg_pio_rw_ce2_cfg___aw___lsb 12
#define reg_pio_rw_ce2_cfg___aw___width 2
#define reg_pio_rw_ce2_cfg___mode___lsb 14
#define reg_pio_rw_ce2_cfg___mode___width 2
#define reg_pio_rw_ce2_cfg_offset 76

/* Register rw_dout, scope pio, type rw */
#define reg_pio_rw_dout___data___lsb 0
#define reg_pio_rw_dout___data___width 8
#define reg_pio_rw_dout___rd_n___lsb 8
#define reg_pio_rw_dout___rd_n___width 1
#define reg_pio_rw_dout___rd_n___bit 8
#define reg_pio_rw_dout___wr_n___lsb 9
#define reg_pio_rw_dout___wr_n___width 1
#define reg_pio_rw_dout___wr_n___bit 9
#define reg_pio_rw_dout___a0___lsb 10
#define reg_pio_rw_dout___a0___width 1
#define reg_pio_rw_dout___a0___bit 10
#define reg_pio_rw_dout___a1___lsb 11
#define reg_pio_rw_dout___a1___width 1
#define reg_pio_rw_dout___a1___bit 11
#define reg_pio_rw_dout___ce0_n___lsb 12
#define reg_pio_rw_dout___ce0_n___width 1
#define reg_pio_rw_dout___ce0_n___bit 12
#define reg_pio_rw_dout___ce1_n___lsb 13
#define reg_pio_rw_dout___ce1_n___width 1
#define reg_pio_rw_dout___ce1_n___bit 13
#define reg_pio_rw_dout___ce2_n___lsb 14
#define reg_pio_rw_dout___ce2_n___width 1
#define reg_pio_rw_dout___ce2_n___bit 14
#define reg_pio_rw_dout___rdy___lsb 15
#define reg_pio_rw_dout___rdy___width 1
#define reg_pio_rw_dout___rdy___bit 15
#define reg_pio_rw_dout_offset 80

/* Register rw_oe, scope pio, type rw */
#define reg_pio_rw_oe___data___lsb 0
#define reg_pio_rw_oe___data___width 8
#define reg_pio_rw_oe___rd_n___lsb 8
#define reg_pio_rw_oe___rd_n___width 1
#define reg_pio_rw_oe___rd_n___bit 8
#define reg_pio_rw_oe___wr_n___lsb 9
#define reg_pio_rw_oe___wr_n___width 1
#define reg_pio_rw_oe___wr_n___bit 9
#define reg_pio_rw_oe___a0___lsb 10
#define reg_pio_rw_oe___a0___width 1
#define reg_pio_rw_oe___a0___bit 10
#define reg_pio_rw_oe___a1___lsb 11
#define reg_pio_rw_oe___a1___width 1
#define reg_pio_rw_oe___a1___bit 11
#define reg_pio_rw_oe___ce0_n___lsb 12
#define reg_pio_rw_oe___ce0_n___width 1
#define reg_pio_rw_oe___ce0_n___bit 12
#define reg_pio_rw_oe___ce1_n___lsb 13
#define reg_pio_rw_oe___ce1_n___width 1
#define reg_pio_rw_oe___ce1_n___bit 13
#define reg_pio_rw_oe___ce2_n___lsb 14
#define reg_pio_rw_oe___ce2_n___width 1
#define reg_pio_rw_oe___ce2_n___bit 14
#define reg_pio_rw_oe___rdy___lsb 15
#define reg_pio_rw_oe___rdy___width 1
#define reg_pio_rw_oe___rdy___bit 15
#define reg_pio_rw_oe_offset 84

/* Register rw_man_ctrl, scope pio, type rw */
#define reg_pio_rw_man_ctrl___data___lsb 0
#define reg_pio_rw_man_ctrl___data___width 8
#define reg_pio_rw_man_ctrl___rd_n___lsb 8
#define reg_pio_rw_man_ctrl___rd_n___width 1
#define reg_pio_rw_man_ctrl___rd_n___bit 8
#define reg_pio_rw_man_ctrl___wr_n___lsb 9
#define reg_pio_rw_man_ctrl___wr_n___width 1
#define reg_pio_rw_man_ctrl___wr_n___bit 9
#define reg_pio_rw_man_ctrl___a0___lsb 10
#define reg_pio_rw_man_ctrl___a0___width 1
#define reg_pio_rw_man_ctrl___a0___bit 10
#define reg_pio_rw_man_ctrl___a1___lsb 11
#define reg_pio_rw_man_ctrl___a1___width 1
#define reg_pio_rw_man_ctrl___a1___bit 11
#define reg_pio_rw_man_ctrl___ce0_n___lsb 12
#define reg_pio_rw_man_ctrl___ce0_n___width 1
#define reg_pio_rw_man_ctrl___ce0_n___bit 12
#define reg_pio_rw_man_ctrl___ce1_n___lsb 13
#define reg_pio_rw_man_ctrl___ce1_n___width 1
#define reg_pio_rw_man_ctrl___ce1_n___bit 13
#define reg_pio_rw_man_ctrl___ce2_n___lsb 14
#define reg_pio_rw_man_ctrl___ce2_n___width 1
#define reg_pio_rw_man_ctrl___ce2_n___bit 14
#define reg_pio_rw_man_ctrl___rdy___lsb 15
#define reg_pio_rw_man_ctrl___rdy___width 1
#define reg_pio_rw_man_ctrl___rdy___bit 15
#define reg_pio_rw_man_ctrl_offset 88

/* Register r_din, scope pio, type r */
#define reg_pio_r_din___data___lsb 0
#define reg_pio_r_din___data___width 8
#define reg_pio_r_din___rd_n___lsb 8
#define reg_pio_r_din___rd_n___width 1
#define reg_pio_r_din___rd_n___bit 8
#define reg_pio_r_din___wr_n___lsb 9
#define reg_pio_r_din___wr_n___width 1
#define reg_pio_r_din___wr_n___bit 9
#define reg_pio_r_din___a0___lsb 10
#define reg_pio_r_din___a0___width 1
#define reg_pio_r_din___a0___bit 10
#define reg_pio_r_din___a1___lsb 11
#define reg_pio_r_din___a1___width 1
#define reg_pio_r_din___a1___bit 11
#define reg_pio_r_din___ce0_n___lsb 12
#define reg_pio_r_din___ce0_n___width 1
#define reg_pio_r_din___ce0_n___bit 12
#define reg_pio_r_din___ce1_n___lsb 13
#define reg_pio_r_din___ce1_n___width 1
#define reg_pio_r_din___ce1_n___bit 13
#define reg_pio_r_din___ce2_n___lsb 14
#define reg_pio_r_din___ce2_n___width 1
#define reg_pio_r_din___ce2_n___bit 14
#define reg_pio_r_din___rdy___lsb 15
#define reg_pio_r_din___rdy___width 1
#define reg_pio_r_din___rdy___bit 15
#define reg_pio_r_din_offset 92

/* Register r_stat, scope pio, type r */
#define reg_pio_r_stat___busy___lsb 0
#define reg_pio_r_stat___busy___width 1
#define reg_pio_r_stat___busy___bit 0
#define reg_pio_r_stat_offset 96

/* Register rw_intr_mask, scope pio, type rw */
#define reg_pio_rw_intr_mask___rdy___lsb 0
#define reg_pio_rw_intr_mask___rdy___width 1
#define reg_pio_rw_intr_mask___rdy___bit 0
#define reg_pio_rw_intr_mask_offset 100

/* Register rw_ack_intr, scope pio, type rw */
#define reg_pio_rw_ack_intr___rdy___lsb 0
#define reg_pio_rw_ack_intr___rdy___width 1
#define reg_pio_rw_ack_intr___rdy___bit 0
#define reg_pio_rw_ack_intr_offset 104

/* Register r_intr, scope pio, type r */
#define reg_pio_r_intr___rdy___lsb 0
#define reg_pio_r_intr___rdy___width 1
#define reg_pio_r_intr___rdy___bit 0
#define reg_pio_r_intr_offset 108

/* Register r_masked_intr, scope pio, type r */
#define reg_pio_r_masked_intr___rdy___lsb 0
#define reg_pio_r_masked_intr___rdy___width 1
#define reg_pio_r_masked_intr___rdy___bit 0
#define reg_pio_r_masked_intr_offset 112


/* Constants */
#define regk_pio_a2                               0x00000003
#define regk_pio_no                               0x00000000
#define regk_pio_normal                           0x00000000
#define regk_pio_rd                               0x00000001
#define regk_pio_rw_ce0_cfg_default               0x00000000
#define regk_pio_rw_ce1_cfg_default               0x00000000
#define regk_pio_rw_ce2_cfg_default               0x00000000
#define regk_pio_rw_intr_mask_default             0x00000000
#define regk_pio_rw_man_ctrl_default              0x00000000
#define regk_pio_rw_oe_default                    0x00000000
#define regk_pio_wr                               0x00000002
#define regk_pio_wr_ce2                           0x00000003
#define regk_pio_yes                              0x00000001
#define regk_pio_yes_all                          0x000000ff
#endif /* __pio_defs_asm_h */
