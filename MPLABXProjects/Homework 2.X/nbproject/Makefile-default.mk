#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS

else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED="HW 2-ways push button.c" "HW 3-Keypad to LCD.c"

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED="${OBJECTDIR}/HW 2-ways push button.o" "${OBJECTDIR}/HW 3-Keypad to LCD.o"
POSSIBLE_DEPFILES="${OBJECTDIR}/HW 2-ways push button.o.d" "${OBJECTDIR}/HW 3-Keypad to LCD.o.d"

# Object Files
OBJECTFILES=${OBJECTDIR}/HW\ 2-ways\ push\ button.o ${OBJECTDIR}/HW\ 3-Keypad\ to\ LCD.o

# Source Files
SOURCEFILES=HW 2-ways push button.c HW 3-Keypad to LCD.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

# The following macros may be used in the pre and post step lines
Device=ATmega328P
ProjectDir="C:\Users\mooha\MPLABXProjects\Homework 2.X"
ProjectName=Homework_2
ConfName=default
ImagePath="dist\default\${IMAGE_TYPE}\Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}"
ImageDir="dist\default\${IMAGE_TYPE}"
ImageName="Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}"
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IsDebug="true"
else
IsDebug="false"
endif

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
	@echo "--------------------------------------"
	@echo "User defined post-build step: ["D:\Microchip\AVRDUDE\avrdude.exe" -C "D:\Microchip\AVRDUDE\avrdude.conf" -c usbasp -p m328p -B 10 -b 115200 -U flash:w:${ImagePath}:i]"
	@"D:\Microchip\AVRDUDE\avrdude.exe" -C "D:\Microchip\AVRDUDE\avrdude.conf" -c usbasp -p m328p -B 10 -b 115200 -U flash:w:${ImagePath}:i
	@echo "--------------------------------------"

MP_PROCESSOR_OPTION=ATmega328P
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/HW\ 2-ways\ push\ button.o: HW\ 2-ways\ push\ button.c  .generated_files/3d3f1f786af3564efbe5e31ec1b0a9063a0a906b.flag .generated_files/b912e95e5e6b3ea8dc1b396ca24601553543063.flag
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} "${OBJECTDIR}/HW 2-ways push button.o".d 
	@${RM} "${OBJECTDIR}/HW 2-ways push button.o" 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1 -g -DDEBUG  -gdwarf-2  -x c -D__$(MP_PROCESSOR_OPTION)__   -mdfp="${DFP_DIR}/xc8"  -Wl,--gc-sections -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -Wall -DXPRJ_default=$(CND_CONF)  $(COMPARISON_BUILD)  -gdwarf-3     -MD -MP -MF "${OBJECTDIR}/HW 2-ways push button.o.d" -MT "${OBJECTDIR}/HW 2-ways push button.o.d" -MT "${OBJECTDIR}/HW 2-ways push button.o" -o "${OBJECTDIR}/HW 2-ways push button.o" "HW 2-ways push button.c" 
	
${OBJECTDIR}/HW\ 3-Keypad\ to\ LCD.o: HW\ 3-Keypad\ to\ LCD.c  .generated_files/44430ea8d24cb7e3c5138d7cd2a5a5e2c95893f1.flag .generated_files/b912e95e5e6b3ea8dc1b396ca24601553543063.flag
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} "${OBJECTDIR}/HW 3-Keypad to LCD.o".d 
	@${RM} "${OBJECTDIR}/HW 3-Keypad to LCD.o" 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1 -g -DDEBUG  -gdwarf-2  -x c -D__$(MP_PROCESSOR_OPTION)__   -mdfp="${DFP_DIR}/xc8"  -Wl,--gc-sections -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -Wall -DXPRJ_default=$(CND_CONF)  $(COMPARISON_BUILD)  -gdwarf-3     -MD -MP -MF "${OBJECTDIR}/HW 3-Keypad to LCD.o.d" -MT "${OBJECTDIR}/HW 3-Keypad to LCD.o.d" -MT "${OBJECTDIR}/HW 3-Keypad to LCD.o" -o "${OBJECTDIR}/HW 3-Keypad to LCD.o" "HW 3-Keypad to LCD.c" 
	
else
${OBJECTDIR}/HW\ 2-ways\ push\ button.o: HW\ 2-ways\ push\ button.c  .generated_files/bb59d5274e3dec7a98cd68a7af7beb007ba85c23.flag .generated_files/b912e95e5e6b3ea8dc1b396ca24601553543063.flag
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} "${OBJECTDIR}/HW 2-ways push button.o".d 
	@${RM} "${OBJECTDIR}/HW 2-ways push button.o" 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -x c -D__$(MP_PROCESSOR_OPTION)__   -mdfp="${DFP_DIR}/xc8"  -Wl,--gc-sections -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -Wall -DXPRJ_default=$(CND_CONF)  $(COMPARISON_BUILD)  -gdwarf-3     -MD -MP -MF "${OBJECTDIR}/HW 2-ways push button.o.d" -MT "${OBJECTDIR}/HW 2-ways push button.o.d" -MT "${OBJECTDIR}/HW 2-ways push button.o" -o "${OBJECTDIR}/HW 2-ways push button.o" "HW 2-ways push button.c" 
	
${OBJECTDIR}/HW\ 3-Keypad\ to\ LCD.o: HW\ 3-Keypad\ to\ LCD.c  .generated_files/267043ca410cf9d16c90cd0ed71e0ad52d455209.flag .generated_files/b912e95e5e6b3ea8dc1b396ca24601553543063.flag
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} "${OBJECTDIR}/HW 3-Keypad to LCD.o".d 
	@${RM} "${OBJECTDIR}/HW 3-Keypad to LCD.o" 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -x c -D__$(MP_PROCESSOR_OPTION)__   -mdfp="${DFP_DIR}/xc8"  -Wl,--gc-sections -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -Wall -DXPRJ_default=$(CND_CONF)  $(COMPARISON_BUILD)  -gdwarf-3     -MD -MP -MF "${OBJECTDIR}/HW 3-Keypad to LCD.o.d" -MT "${OBJECTDIR}/HW 3-Keypad to LCD.o.d" -MT "${OBJECTDIR}/HW 3-Keypad to LCD.o" -o "${OBJECTDIR}/HW 3-Keypad to LCD.o" "HW 3-Keypad to LCD.c" 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"   -gdwarf-2 -Wl,--gc-sections -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -Wall -gdwarf-3     $(COMPARISON_BUILD) -Wl,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -o dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  -o dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -Wl,--start-group  -Wl,-lm -Wl,--end-group  -Wl,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1
	@${RM} dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.hex 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -Wl,--gc-sections -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -Wall -gdwarf-3     $(COMPARISON_BUILD) -Wl,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -o dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  -o dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -Wl,--start-group  -Wl,-lm -Wl,--end-group 
	${MP_CC_DIR}\\avr-objcopy -O ihex "dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}" "dist/${CND_CONF}/${IMAGE_TYPE}/Homework_2.X.${IMAGE_TYPE}.hex"
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
