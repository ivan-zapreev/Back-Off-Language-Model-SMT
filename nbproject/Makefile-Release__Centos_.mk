#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=/opt/rh/devtoolset-3/root/usr/bin/g++
CXX=/opt/rh/devtoolset-3/root/usr/bin/g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release__Centos_
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/ARPAGramBuilder.o \
	${OBJECTDIR}/src/ARPATrieBuilder.o \
	${OBJECTDIR}/src/ATrie.o \
	${OBJECTDIR}/src/C2WOrderedArrayTrie.o \
	${OBJECTDIR}/src/CtxMultiHashMapTrie.o \
	${OBJECTDIR}/src/Logger.o \
	${OBJECTDIR}/src/StatisticsMonitor.o \
	${OBJECTDIR}/src/W2CHybridMemoryTrie.o \
	${OBJECTDIR}/src/W2COrderedArrayTrie.o \
	${OBJECTDIR}/src/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-O3 -std=c++0x -lrt -m64
CXXFLAGS=-O3 -std=c++0x -lrt -m64

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt

${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}
	/opt/rh/devtoolset-3/root/usr/bin/g++ -o ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt ${OBJECTFILES} ${LDLIBSOPTIONS} -lrt -m64

${OBJECTDIR}/src/ARPAGramBuilder.o: nbproject/Makefile-${CND_CONF}.mk src/ARPAGramBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPAGramBuilder.o src/ARPAGramBuilder.cpp

${OBJECTDIR}/src/ARPATrieBuilder.o: nbproject/Makefile-${CND_CONF}.mk src/ARPATrieBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPATrieBuilder.o src/ARPATrieBuilder.cpp

${OBJECTDIR}/src/ATrie.o: nbproject/Makefile-${CND_CONF}.mk src/ATrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ATrie.o src/ATrie.cpp

${OBJECTDIR}/src/C2WOrderedArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2WOrderedArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2WOrderedArrayTrie.o src/C2WOrderedArrayTrie.cpp

${OBJECTDIR}/src/CtxMultiHashMapTrie.o: nbproject/Makefile-${CND_CONF}.mk src/CtxMultiHashMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/CtxMultiHashMapTrie.o src/CtxMultiHashMapTrie.cpp

${OBJECTDIR}/src/Logger.o: nbproject/Makefile-${CND_CONF}.mk src/Logger.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Logger.o src/Logger.cpp

${OBJECTDIR}/src/StatisticsMonitor.o: nbproject/Makefile-${CND_CONF}.mk src/StatisticsMonitor.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/StatisticsMonitor.o src/StatisticsMonitor.cpp

${OBJECTDIR}/src/W2CHybridMemoryTrie.o: nbproject/Makefile-${CND_CONF}.mk src/W2CHybridMemoryTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2CHybridMemoryTrie.o src/W2CHybridMemoryTrie.cpp

${OBJECTDIR}/src/W2COrderedArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/W2COrderedArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2COrderedArrayTrie.o src/W2COrderedArrayTrie.cpp

${OBJECTDIR}/src/main.o: nbproject/Makefile-${CND_CONF}.mk src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
