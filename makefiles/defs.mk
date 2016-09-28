define SRCOBJ
$(patsubst $(SRCDIR)/%.cpp, $(OOBJDIR)/%.o, $(patsubst $(SRCDIR)/%.s, $(OOBJDIR)/%.o, $(1)))
endef

define DEPSRC
DEPS += $$(patsubst %.o,%.d,$$(call SRCOBJ,$(1)))
endef

define SCANMOD
MOD_$(1)_SRCS := $$(wildcard $$(MODDIR)/$(1)/*.cpp)
MODSRCS := $$(MODSRCS) $$(MOD_$(1)_SRCS)
$$(OOBJDIR)/mod_$(1).o: $$(MOD_$(1)_SRCS)
	@ mkdir -p $$(dir $$@)
	@ echo MODCC $(1)
	@ $$(CC) $$(CFLAGS) -c $$^ -o $$@
endef
