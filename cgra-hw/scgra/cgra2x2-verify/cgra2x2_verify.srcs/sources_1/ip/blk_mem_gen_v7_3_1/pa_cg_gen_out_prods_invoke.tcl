# Tcl script generated by PlanAhead

set reloadAllCoreGenRepositories false

set tclUtilsPath "c:/EDA/14.7/ISE_DS/PlanAhead/scripts/pa_cg_utils.tcl"

set repoPaths ""

set cgIndexMapPath "D:/minibench/scgra/cgra2x2_verify/cgra2x2_verify.srcs/sources_1/ip/cg_nt_index_map.xml"

set cgProjectPath "d:/minibench/scgra/cgra2x2_verify/cgra2x2_verify.srcs/sources_1/ip/blk_mem_gen_v7_3_1/coregen.cgc"

set ipFile "d:/minibench/scgra/cgra2x2_verify/cgra2x2_verify.srcs/sources_1/ip/blk_mem_gen_v7_3_1/rom01.xco"

set ipName "rom01"

set hdlType "Verilog"

set cgPartSpec "xc7vx485t-1ffg1157"

set chains "GENERATE_CURRENT_CHAIN"

set params ""

set bomFilePath "d:/minibench/scgra/cgra2x2_verify/cgra2x2_verify.srcs/sources_1/ip/blk_mem_gen_v7_3_1/pa_cg_bom.xml"

# generate the IP
set result [source "c:/EDA/14.7/ISE_DS/PlanAhead/scripts/pa_cg_gen_out_prods.tcl"]

exit $result

