define SCANDIRS
$(foreach f,$(wildcard $(1)/$(2)) $(foreach s,$(wildcard $(1)/*),$(call SCANDIRS,$(s),$(2))),$(dir $(f)))
endef

define UNIQ
$(if $1,$(firstword $1) $(call UNIQ,$(filter-out $(firstword $1),$1)))
endef

define SRCOBJ
$(patsubst $(SRCDIR)/%,$(OOBJDIR)/%,$(patsubst %.cpp,%.o,$(patsubst %.s,%.o,$(1))))
endef

define DEPSRC
$(patsubst %.o,%.d,$(call SRCOBJ,$(1)))
endef

define RULESCAN
LIBNAME_$(1) := $$(subst /,_,$$(patsubst %/,%,$(1)))
CSRCS_$(1) := $$(wildcard $(SRCDIR)/$(1)*.cpp)
ASRCS_$(1) := $$(wildcard $(SRCDIR)/$(1)*.s)
SRCS_$(1) := $$(CSRCS_$(1)) $$(ASRCS_$(1))
OBJS_$(1) := $$(call SRCOBJ,$$(SRCS_$(1)))
INCLUDES_$(1) := $$(wildcard $(SRCDIR)/$(1)include)
INCLUDES += $$(INCLUDES_$(1))
SOURCES += $$(SRCS_$(1))
endef

define KERNRULES
$(call RULESCAN,$(1))
KERNOBJS += $(OLIBDIR)/$$(LIBNAME_$(1)).a

$(OLIBDIR)/$$(LIBNAME_$(1)).a: $$(OBJS_$(1))
	@ mkdir -p $$(dir $$@)
	$(QECHO) AR $$(subst $(OLIBDIR)/,,$$@)
	$(Q) $(AR) cru $$@ $$?
endef

define LIBRULES
LIBOBJS += $(OLIBDIR)/$$(patsubst %/,%,$(1)).a
endef

define MODRULES
$(call RULESCAN,$(1))
MODOBJS += $(OOBJDIR)/mod_$$(LIBNAME_$(1)).o

$(OOBJDIR)/mod_$$(LIBNAME_$(1)).o: $$(OBJS_$(1))
	@ mkdir -p $$(dir $$@)
	$(QECHO) MODLD $$(patsubst %/,%,$(1))
	$(Q) $(LD) $(LDFLAGS) --shared -s -o $$@ $$?
endef
