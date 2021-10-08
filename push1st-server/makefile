rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SOURCES += $(call rwildcard,../lib/http/,*.cpp)
SOURCES += $(call rwildcard,../lib/inet/,*.cpp)
SOURCES += $(call rwildcard,push1st-server/,*.cpp)

INC                                     = -I ../lib/http -I ../lib/lua
CC                                      = g++
CC_FLAGS                                = -W"no-format" -std=c++17 -fno-strict-aliasing
CC_DFLAGS                               = ${INC}
LIBS                                    = -lcrypto -lssl -llua5.3 -static-libgcc -static-libstdc++ \
                                        -Wl,-Bstatic -lyaml-cpp \
                                        -Wl,-Bdynamic -pthread


VERSION_NAME                            = $(shell git describe `git rev-list --tags --max-count=1`)
REV_NAME                                = $(shell git rev-parse --short HEAD)
BRANCH_NAME                             = $(shell git rev-parse --abbrev-ref HEAD)
TIMESTAMP                               = $(shell date "+%Y%m%d%H%M%S")
BUILD_NO                                = $(build)
BUILD_TS								= $(shell date "+%y%j")

ifeq ($(BUILD_NO),)
	BUILD_NO := 0
endif

ifeq ($(VERSION_NAME),)
	VERSION_NAME := $(tag)
endif

ifeq ($(REV_NAME),)
	REV_NAME := ${TIMESTAMP}
endif

ifeq ($(BRANCH_NAME),)
	BRANCH_NAME := dev
endif



BIN_EXEC   = push1st-${VERSION_NAME}-${BRANCH_NAME}
BIN_DIR    = build
OBJ_DIR    = build/${BRANCH_NAME}
PROD_FLAGS = -O3
DBG_FLAGS  = -O0 -ggdb
LINK_FLAGS = ${PROD_FLAGS} -fPIC -L/usr/local/lib

all: version branch archive deb

rebuild: clean version branch archive deb

branch: $(shell mkdir -p ${OBJ_DIR} && mkdir -p ${BIN_DIR})
branch: $(patsubst %.cpp,%.o,$(SOURCES))
	@${CC} ${LINK_FLAGS} $(addprefix ${OBJ_DIR}/,$(notdir $^)) -o ${BIN_DIR}/${BIN_EXEC} ${LIBS}
	@echo "\033[1;32mlink    \033[0m ${BIN_DIR}/${BIN_EXEC}"

%.o: %.cpp
ifeq (,$(wildcard $@))
	@${CC} ${CC_FLAGS} ${LINK_FLAGS} -c $^ -o $(addprefix ${OBJ_DIR}/,$(notdir $@)); r=$$?;
ifneq ($$r,0)
	@echo "\033[1;90mbuild    \033[0m ${^} ... \033[1;32msuccess\033[0m"
else
	@echo "\033[1;90mbuild    \033[0m ${^} ... \033[1;31mfail\033[0m"
endif
else
	@echo "${^}"
endif

clean:
	@echo "Cleaning... ../build/${BRANCH_NAME}"
	rm -rf ../build/${BRANCH_NAME}
	rm -rf ${BIN_DIR}/${BIN_EXEC}

version:
	@echo "#include \"version.h\"\nconst char* version::number{ \"${BUILD_TS}.${BUILD_NO}\" };\nconst char* version::build{ \"${REV_NAME}\" };\nconst char* version::revision{ \"${VERSION_NAME}-${BRANCH_NAME}\" };\nconst char* version::timestamp{ \"${TIMESTAMP}\" };"  > ./version.cpp

deb: 
	sudo chmod +x ./makedeb
	./makedeb ${BUILD_TS}.${BUILD_NO} ${VERSION_NAME} ${BRANCH_NAME}
	@echo "\033[1;32mmake deb \033[0m ${BIN_DIR}/naveksoft-beam.deb"

archive:
	mkdir -p scripts
	cp build/${BIN_EXEC} beam-server
	cp events-hook.lua scripts/events-hook.lua
	cp -n server.yaml server.example.yaml
	tar cvjf ${BIN_DIR}/naveksoft-beam.tar.bz2 beam-server server.example.yaml scripts/events-hook.lua
	@echo "\033[1;32mmake tar \033[0m ${BIN_DIR}/naveksoft-beam.tar.bz2"