#################################################################
#								#
#	Copyright 2013 Fidelity Information Services, Inc	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################
set(I "")
foreach(i ${includes})
  list(APPEND I "-I${i}")
endforeach()
file(WRITE tmp_xfer_1.c "
/* We have not yet created gtm_threadgbl_deftypes.h and don't need it, signal gtm_threadgbl.h to avoid including it */
#define NO_THREADGBL_DEFTYPES
#include \"mdef.h\"
#define XFER(a,b) MY_XF,b
#include \"xfer.h\"
")

execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${I} -E tmp_xfer_1.c -o tmp_xfer_2.c
  RESULT_VARIABLE failed
  )
if(failed)
  message(FATAL_ERROR "Preprocessing with ${CMAKE_C_COMPILER} failed")
endif()
file(STRINGS tmp_xfer_2.c lines REGEX "MY_XF")
string(REGEX REPLACE "(MY_XF|,)" "" names "${lines}")
file(REMOVE tmp_xfer_1.c tmp_xfer_2.c)

if("${arch}" MATCHES "ia64")
  # Guess what! Its possible for xfer_table to be initialized by functions others than
  # then the ones in xfer.h. So append those names explicitly here
  list(APPEND names
    op_fnzreverse
    op_zst_st_over
    op_zst_fet_over
    op_zstzb_fet_over
    op_zstzb_st_over
    opp_zstepret
    opp_zstepretarg
    op_zstepfetch
    op_zstepstart
    op_zstzbfetch
    op_zstzbstart
    opp_zst_over_ret
    opp_zst_over_retarg
    op_fetchintrrpt
    op_startintrrpt
    op_forintrrpt
    op_mprofextexfun
    op_mprofextcall
    op_mprofcalll
    op_mprofcallb
    op_mprofcallw
    op_mprofcallspl
    op_mprofcallspb
    op_mprofcallspw
    op_mprofexfun
    op_mprofforlcldow
    op_mprofforlcldol
    op_mprofforlcldob
    op_mprofforchk1
    op_mproflinefetch
    op_mproflinestart
  )
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sources.list" sources)

# On X86_64, the calling convention for variable length functions defines that the register $RAX
# (ie the lower 8 bytes of this register actually) will contain the # of floating point arguments passed to that function.
# $RAX is not an argument register normally and so is typically unused (and also is not expected to be preserved across calls).
# since none of the XFER functions actually take double/float arguments, but some are variable length functions,
# the generated code should set the $RAX register to the value '0'.
# Theoretically this register can be set to 0 all the time during a function call.
# But to optimize the number of generated instructions, identify which XFER function is actually a var arg function
# And if the logic to identify it falls thru, blindly assume its a VAR_ARG function.
# Hence the logic below might incorrectly mark a few functions like op_sub, op_fnzascii as C_VAR_ARGS. But that is okay.

set(ftypes "")
set(defines "\n/*  Defines used in resetting xfer_table_desc on transfer table changes */\n")
foreach(name ${names})
  set(ftype "")
  if(";${sources};" MATCHES ";([^;]*/${name}\\.s);")
    set(ftype GTM_ASM_RTN)
  elseif(";${sources};" MATCHES ";([^;]*/${name}\\.c);")
    file(STRINGS "${CMAKE_MATCH_1}" sig REGEX "${name}.*\\.\\.\\.")
    if(sig)
      set(ftype "GTM_C_VAR_ARGS_RTN")
    else()
      set(ftype "GTM_C_RTN")
    endif()
  endif()
  if(NOT ftype)
    set(ftype "GTM_C_VAR_ARGS_RTN")
    foreach(src ${sources})
      if("${src}" MATCHES "\\.s$")
        file(STRINGS "${src}" sig REGEX "^${name}")
        if(sig)
          set(ftype "GTM_ASM_RTN")
          break()
        endif()
      endif()
    endforeach()
  endif()
  set(ftypes "${ftypes}${ftype}, /* ${name} abc */ \\\n")
  set(defines "${defines}#define ${name}_FUNCTYPE ${ftype}\n")
endforeach()

if("${arch}" MATCHES "ia64")
else()
  # Not IA64? reset defines
  set(defines "")
endif()

file(WRITE xfer_desc.i "/* Generated by gen_xfer_desc.cmake */
#define GTM_C_RTN 1
#define GTM_ASM_RTN 2
#define GTM_C_VAR_ARGS_RTN 3
#define DEFINE_XFER_TABLE_DESC char xfer_table_desc[] = \\
{ \\
${ftypes}0}\n
${defines}")
