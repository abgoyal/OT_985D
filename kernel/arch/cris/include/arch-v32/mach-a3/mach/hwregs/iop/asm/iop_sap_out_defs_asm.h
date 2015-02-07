
#ifndef __iop_sap_out_defs_asm_h
#define __iop_sap_out_defs_asm_h


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

/* Register rw_gen_gated, scope iop_sap_out, type rw */
#define reg_iop_sap_out_rw_gen_gated___clk0_src___lsb 0
#define reg_iop_sap_out_rw_gen_gated___clk0_src___width 2
#define reg_iop_sap_out_rw_gen_gated___clk0_gate_src___lsb 2
#define reg_iop_sap_out_rw_gen_gated___clk0_gate_src___width 2
#define reg_iop_sap_out_rw_gen_gated___clk0_force_src___lsb 4
#define reg_iop_sap_out_rw_gen_gated___clk0_force_src___width 3
#define reg_iop_sap_out_rw_gen_gated___clk1_src___lsb 7
#define reg_iop_sap_out_rw_gen_gated___clk1_src___width 2
#define reg_iop_sap_out_rw_gen_gated___clk1_gate_src___lsb 9
#define reg_iop_sap_out_rw_gen_gated___clk1_gate_src___width 2
#define reg_iop_sap_out_rw_gen_gated___clk1_force_src___lsb 11
#define reg_iop_sap_out_rw_gen_gated___clk1_force_src___width 3
#define reg_iop_sap_out_rw_gen_gated_offset 0

/* Register rw_bus, scope iop_sap_out, type rw */
#define reg_iop_sap_out_rw_bus___byte0_clk_sel___lsb 0
#define reg_iop_sap_out_rw_bus___byte0_clk_sel___width 2
#define reg_iop_sap_out_rw_bus___byte0_clk_ext___lsb 2
#define reg_iop_sap_out_rw_bus___byte0_clk_ext___width 2
#define reg_iop_sap_out_rw_bus___byte0_gated_clk___lsb 4
#define reg_iop_sap_out_rw_bus___byte0_gated_clk___width 1
#define reg_iop_sap_out_rw_bus___byte0_gated_clk___bit 4
#define reg_iop_sap_out_rw_bus___byte0_clk_inv___lsb 5
#define reg_iop_sap_out_rw_bus___byte0_clk_inv___width 1
#define reg_iop_sap_out_rw_bus___byte0_clk_inv___bit 5
#define reg_iop_sap_out_rw_bus___byte0_delay___lsb 6
#define reg_iop_sap_out_rw_bus___byte0_delay___width 1
#define reg_iop_sap_out_rw_bus___byte0_delay___bit 6
#define reg_iop_sap_out_rw_bus___byte1_clk_sel___lsb 7
#define reg_iop_sap_out_rw_bus___byte1_clk_sel___width 2
#define reg_iop_sap_out_rw_bus___byte1_clk_ext___lsb 9
#define reg_iop_sap_out_rw_bus___byte1_clk_ext___width 2
#define reg_iop_sap_out_rw_bus___byte1_gated_clk___lsb 11
#define reg_iop_sap_out_rw_bus___byte1_gated_clk___width 1
#define reg_iop_sap_out_rw_bus___byte1_gated_clk___bit 11
#define reg_iop_sap_out_rw_bus___byte1_clk_inv___lsb 12
#define reg_iop_sap_out_rw_bus___byte1_clk_inv___width 1
#define reg_iop_sap_out_rw_bus___byte1_clk_inv___bit 12
#define reg_iop_sap_out_rw_bus___byte1_delay___lsb 13
#define reg_iop_sap_out_rw_bus___byte1_delay___width 1
#define reg_iop_sap_out_rw_bus___byte1_delay___bit 13
#define reg_iop_sap_out_rw_bus___byte2_clk_sel___lsb 14
#define reg_iop_sap_out_rw_bus___byte2_clk_sel___width 2
#define reg_iop_sap_out_rw_bus___byte2_clk_ext___lsb 16
#define reg_iop_sap_out_rw_bus___byte2_clk_ext___width 2
#define reg_iop_sap_out_rw_bus___byte2_gated_clk___lsb 18
#define reg_iop_sap_out_rw_bus___byte2_gated_clk___width 1
#define reg_iop_sap_out_rw_bus___byte2_gated_clk___bit 18
#define reg_iop_sap_out_rw_bus___byte2_clk_inv___lsb 19
#define reg_iop_sap_out_rw_bus___byte2_clk_inv___width 1
#define reg_iop_sap_out_rw_bus___byte2_clk_inv___bit 19
#define reg_iop_sap_out_rw_bus___byte2_delay___lsb 20
#define reg_iop_sap_out_rw_bus___byte2_delay___width 1
#define reg_iop_sap_out_rw_bus___byte2_delay___bit 20
#define reg_iop_sap_out_rw_bus___byte3_clk_sel___lsb 21
#define reg_iop_sap_out_rw_bus___byte3_clk_sel___width 2
#define reg_iop_sap_out_rw_bus___byte3_clk_ext___lsb 23
#define reg_iop_sap_out_rw_bus___byte3_clk_ext___width 2
#define reg_iop_sap_out_rw_bus___byte3_gated_clk___lsb 25
#define reg_iop_sap_out_rw_bus___byte3_gated_clk___width 1
#define reg_iop_sap_out_rw_bus___byte3_gated_clk___bit 25
#define reg_iop_sap_out_rw_bus___byte3_clk_inv___lsb 26
#define reg_iop_sap_out_rw_bus___byte3_clk_inv___width 1
#define reg_iop_sap_out_rw_bus___byte3_clk_inv___bit 26
#define reg_iop_sap_out_rw_bus___byte3_delay___lsb 27
#define reg_iop_sap_out_rw_bus___byte3_delay___width 1
#define reg_iop_sap_out_rw_bus___byte3_delay___bit 27
#define reg_iop_sap_out_rw_bus_offset 4

/* Register rw_bus_lo_oe, scope iop_sap_out, type rw */
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_sel___lsb 0
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_sel___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_ext___lsb 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_ext___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_gated_clk___lsb 4
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_gated_clk___width 1
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_gated_clk___bit 4
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_inv___lsb 5
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_inv___width 1
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_clk_inv___bit 5
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_delay___lsb 6
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_delay___width 1
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_delay___bit 6
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_logic___lsb 7
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_logic___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_logic_src___lsb 9
#define reg_iop_sap_out_rw_bus_lo_oe___byte0_logic_src___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_sel___lsb 11
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_sel___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_ext___lsb 13
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_ext___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_gated_clk___lsb 15
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_gated_clk___width 1
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_gated_clk___bit 15
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_inv___lsb 16
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_inv___width 1
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_clk_inv___bit 16
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_delay___lsb 17
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_delay___width 1
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_delay___bit 17
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_logic___lsb 18
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_logic___width 2
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_logic_src___lsb 20
#define reg_iop_sap_out_rw_bus_lo_oe___byte1_logic_src___width 2
#define reg_iop_sap_out_rw_bus_lo_oe_offset 8

/* Register rw_bus_hi_oe, scope iop_sap_out, type rw */
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_sel___lsb 0
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_sel___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_ext___lsb 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_ext___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_gated_clk___lsb 4
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_gated_clk___width 1
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_gated_clk___bit 4
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_inv___lsb 5
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_inv___width 1
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_clk_inv___bit 5
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_delay___lsb 6
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_delay___width 1
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_delay___bit 6
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_logic___lsb 7
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_logic___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_logic_src___lsb 9
#define reg_iop_sap_out_rw_bus_hi_oe___byte2_logic_src___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_sel___lsb 11
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_sel___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_ext___lsb 13
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_ext___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_gated_clk___lsb 15
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_gated_clk___width 1
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_gated_clk___bit 15
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_inv___lsb 16
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_inv___width 1
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_clk_inv___bit 16
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_delay___lsb 17
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_delay___width 1
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_delay___bit 17
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_logic___lsb 18
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_logic___width 2
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_logic_src___lsb 20
#define reg_iop_sap_out_rw_bus_hi_oe___byte3_logic_src___width 2
#define reg_iop_sap_out_rw_bus_hi_oe_offset 12

#define STRIDE_iop_sap_out_rw_gio 4
/* Register rw_gio, scope iop_sap_out, type rw */
#define reg_iop_sap_out_rw_gio___out_clk_sel___lsb 0
#define reg_iop_sap_out_rw_gio___out_clk_sel___width 3
#define reg_iop_sap_out_rw_gio___out_clk_ext___lsb 3
#define reg_iop_sap_out_rw_gio___out_clk_ext___width 2
#define reg_iop_sap_out_rw_gio___out_gated_clk___lsb 5
#define reg_iop_sap_out_rw_gio___out_gated_clk___width 1
#define reg_iop_sap_out_rw_gio___out_gated_clk___bit 5
#define reg_iop_sap_out_rw_gio___out_clk_inv___lsb 6
#define reg_iop_sap_out_rw_gio___out_clk_inv___width 1
#define reg_iop_sap_out_rw_gio___out_clk_inv___bit 6
#define reg_iop_sap_out_rw_gio___out_delay___lsb 7
#define reg_iop_sap_out_rw_gio___out_delay___width 1
#define reg_iop_sap_out_rw_gio___out_delay___bit 7
#define reg_iop_sap_out_rw_gio___out_logic___lsb 8
#define reg_iop_sap_out_rw_gio___out_logic___width 2
#define reg_iop_sap_out_rw_gio___out_logic_src___lsb 10
#define reg_iop_sap_out_rw_gio___out_logic_src___width 2
#define reg_iop_sap_out_rw_gio___oe_clk_sel___lsb 12
#define reg_iop_sap_out_rw_gio___oe_clk_sel___width 3
#define reg_iop_sap_out_rw_gio___oe_clk_ext___lsb 15
#define reg_iop_sap_out_rw_gio___oe_clk_ext___width 2
#define reg_iop_sap_out_rw_gio___oe_gated_clk___lsb 17
#define reg_iop_sap_out_rw_gio___oe_gated_clk___width 1
#define reg_iop_sap_out_rw_gio___oe_gated_clk___bit 17
#define reg_iop_sap_out_rw_gio___oe_clk_inv___lsb 18
#define reg_iop_sap_out_rw_gio___oe_clk_inv___width 1
#define reg_iop_sap_out_rw_gio___oe_clk_inv___bit 18
#define reg_iop_sap_out_rw_gio___oe_delay___lsb 19
#define reg_iop_sap_out_rw_gio___oe_delay___width 1
#define reg_iop_sap_out_rw_gio___oe_delay___bit 19
#define reg_iop_sap_out_rw_gio___oe_logic___lsb 20
#define reg_iop_sap_out_rw_gio___oe_logic___width 2
#define reg_iop_sap_out_rw_gio___oe_logic_src___lsb 22
#define reg_iop_sap_out_rw_gio___oe_logic_src___width 2
#define reg_iop_sap_out_rw_gio_offset 16


/* Constants */
#define regk_iop_sap_out_always                   0x00000001
#define regk_iop_sap_out_and                      0x00000002
#define regk_iop_sap_out_clk0                     0x00000000
#define regk_iop_sap_out_clk1                     0x00000001
#define regk_iop_sap_out_clk12                    0x00000004
#define regk_iop_sap_out_clk200                   0x00000000
#define regk_iop_sap_out_ext                      0x00000002
#define regk_iop_sap_out_gated                    0x00000003
#define regk_iop_sap_out_gio0                     0x00000000
#define regk_iop_sap_out_gio1                     0x00000000
#define regk_iop_sap_out_gio16                    0x00000002
#define regk_iop_sap_out_gio17                    0x00000002
#define regk_iop_sap_out_gio24                    0x00000003
#define regk_iop_sap_out_gio25                    0x00000003
#define regk_iop_sap_out_gio8                     0x00000001
#define regk_iop_sap_out_gio9                     0x00000001
#define regk_iop_sap_out_gio_out10                0x00000005
#define regk_iop_sap_out_gio_out18                0x00000006
#define regk_iop_sap_out_gio_out2                 0x00000004
#define regk_iop_sap_out_gio_out26                0x00000007
#define regk_iop_sap_out_inv                      0x00000001
#define regk_iop_sap_out_nand                     0x00000003
#define regk_iop_sap_out_no                       0x00000000
#define regk_iop_sap_out_none                     0x00000000
#define regk_iop_sap_out_one                      0x00000001
#define regk_iop_sap_out_rw_bus_default           0x00000000
#define regk_iop_sap_out_rw_bus_hi_oe_default     0x00000000
#define regk_iop_sap_out_rw_bus_lo_oe_default     0x00000000
#define regk_iop_sap_out_rw_gen_gated_default     0x00000000
#define regk_iop_sap_out_rw_gio_default           0x00000000
#define regk_iop_sap_out_rw_gio_size              0x00000020
#define regk_iop_sap_out_spu_gio6                 0x00000002
#define regk_iop_sap_out_spu_gio7                 0x00000003
#define regk_iop_sap_out_timer_grp0_tmr2          0x00000000
#define regk_iop_sap_out_timer_grp0_tmr3          0x00000001
#define regk_iop_sap_out_timer_grp1_tmr2          0x00000002
#define regk_iop_sap_out_timer_grp1_tmr3          0x00000003
#define regk_iop_sap_out_tmr200                   0x00000001
#define regk_iop_sap_out_yes                      0x00000001
#endif /* __iop_sap_out_defs_asm_h */
