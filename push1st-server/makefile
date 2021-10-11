rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SOURCES += $(call rwildcard,../lib/http/,*.cpp)
SOURCES += $(call rwildcard,../lib/inet/,*.cpp)
SOURCES += $(call rwildcard,../push1st-server/,*.cpp)

INC                                     = -I ../lib/http -I ../lib/inet
CC                                      = g++
CC_FLAGS                                = -W"no-format" -std=c++17 -fno-strict-aliasing
CC_DFLAGS                               = ${INC}
LIBS                                    = -lcrypto -lssl -llua5.3 -static-libgcc -static-libstdc++ \
										-Wl,-Bstatic -lyaml-cpp \
										-Wl,-Bdynamic -pthread


VERSION_NAME                            = $(shell git describe --tags `git rev-list --tags --max-count=1`)
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

ifeq ($(VERSION_NAME),)
	VERSION_NAME := 0.1
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
	@echo "#include \"version.h\"\nconst char* version::number{ \"${VERSION_NAME}.${BUILD_NO}\" };\nconst char* version::build{ \"${REV_NAME}\" };\nconst char* version::revision{ \"${VERSION_NAME}-${BRANCH_NAME}\" };\nconst char* version::timestamp{ \"${TIMESTAMP}\" };"
	@echo "#include \"version.h\"\nconst char* version::number{ \"${VERSION_NAME}.${BUILD_NO}\" };\nconst char* version::build{ \"${REV_NAME}\" };\nconst char* version::revision{ \"${VERSION_NAME}-${BRANCH_NAME}\" };\nconst char* version::timestamp{ \"${TIMESTAMP}\" };"  > ./version.cpp

deb: 
	sudo chmod +x ./makedeb
	./makedeb push1st package-${BRANCH_NAME} ${BUILD_TS}.${BUILD_NO} ${VERSION_NAME} ${BRANCH_NAME}
	@echo "\033[1;32mmake deb \033[0m push1st-${BRANCH_NAME}.deb"

archive:
	mkdir -p package-${BRANCH_NAME}/scripts package-${BRANCH_NAME}/apps
	cp build/${BIN_EXEC} package-${BRANCH_NAME}/push1st
	cp -n opt/server.example.yml package-${BRANCH_NAME}/server.example.yml
	cp -n opt/apps/app.example.yml package-${BRANCH_NAME}/apps/app.example.yml
	tar -cvjf push1st-${BRANCH_NAME}.tar.bz2 --transform='s/package-${BRANCH_NAME}\///g'  package-${BRANCH_NAME}/*
	@echo "\033[1;32mmake tar \033[0m push1st-${BRANCH_NAME}.tar.bz2"