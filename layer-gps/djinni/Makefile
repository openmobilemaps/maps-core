
#Use bash as shell
SHELL := /bin/bash

notarget:
	@echo Please specify a target!

#################
# DJINNI TARGET #
#################

DJINNI_OUT_DIR := ../bridging
DJINNI_SOURCE := ./$(FILE)

PROJECT_NAME := gps_shared
PROJECT_PREFIX := MC
JAVA_PACKAGE := io.openmobilemaps.gps.shared

OBJC_OUT := $(DJINNI_OUT_DIR)/ios
OBJC_PREFIX := $(PROJECT_PREFIX)
OBJCPP_OUT := $(DJINNI_OUT_DIR)/ios
UB_FOUNDATION_HEADER := '<Foundation/Foundation.h>'

YAML_ARG := --yaml-out ./yaml

DJINNI_RUN := ../external/djinni/src/run

JAVA_OUT := $(DJINNI_OUT_DIR)/android/java/io/openmobilemaps/gps/shared
JNI_OUT := $(DJINNI_OUT_DIR)/android/jni
IDENT_JNI_CLASS := NativeFooBar
IDENT_JAVA := fooBar

CPP_OUT := ../shared/public
HPP_EXT := h
IDENT_CPP := FooBar
IDENT_CPP_FIELD := fooBar
IDENT_CPP_ENUM := FOO_BAR
IDENT_CPP_METHOD := fooBar

PRODUCTDIR := ./build/djinni/products

DJINNISRCDIR := .
DJINNISRC := $(shell find $(DJINNISRCDIR) -type f -name '*.djinni')
DJINNILOG := $(patsubst $(DJINNISRCDIR)/%.djinni, $(DJINNI_OUT_DIR)/%.log, $(DJINNISRC))
$(DJINNI_OUT_DIR)/%.log: $(DJINNISRCDIR)/%.djinni
	@echo Compiling file $<
	$(eval SUB := $(shell echo "$<" | sed -n "s/\(.*\)\/[^\/]*.djinni/\1/p"))
	$(eval SUBPOINT := $(subst /,.,$(SUB)))
	$(eval SUBPOINT := $(shell if [ "$(SUBPOINT)" == "" ];then echo ""; else echo ".$(SUBPOINT)"; fi ))

	mkdir -p $(DJINNI_OUT_DIR)

	$(DJINNI_RUN) --idl ../djinni/$< \
	--kotlin-out $(JAVA_OUT)/$(SUB) \
	--jni-out $(JNI_OUT)/$(SUB) \
	--objc-out $(OBJC_OUT)/ \
	--cpp-out $(CPP_OUT)/ \
	--ident-jni-class $(IDENT_JNI_CLASS) \
	--objc-type-prefix $(OBJC_PREFIX) \
	--objcpp-out $(OBJCPP_OUT)/ \
	--hpp-ext $(HPP_EXT) \
	--ident-cpp-type $(IDENT_CPP) \
	--ident-cpp-file $(IDENT_CPP) \
	--ident-cpp-enum $(IDENT_CPP_ENUM) \
	--ident-cpp-type-param $(IDENT_CPP_METHOD) \
	--ident-cpp-method $(IDENT_CPP_METHOD) \
	--ident-cpp-local $(IDENT_CPP_METHOD) \
	--ident-cpp-field $(IDENT_CPP_FIELD) \
	--ident-java-field $(IDENT_JAVA) \
	--ident-java-enum $(IDENT_CPP_ENUM) \
	--ident-objc-enum $(IDENT_CPP_ENUM) \
	--java-package $(JAVA_PACKAGE)$(SUBPOINT) \
	--ident-jni-file $(IDENT_JNI_CLASS) $(YAML_ARG) \
	--ub-foundation-header $(UB_FOUNDATION_HEADER)

djinni: $(DJINNILOG)
	@echo Djinni finished

clean:
	rm -rf ../bridging

install:
	@echo Nothing to install
	true
