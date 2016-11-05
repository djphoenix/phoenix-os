KERNEL_DIRS := $(subst $(SRCDIR)/,,$(call SCANDIRS,$(SRCDIR),kernel.mk))
LIBRARY_DIRS := $(subst $(SRCDIR)/,,$(call SCANDIRS,$(SRCDIR),library.mk))
MODULE_DIRS := $(subst $(SRCDIR)/,,$(call SCANDIRS,$(SRCDIR),module.mk))

$(foreach dir,$(KERNEL_DIRS),$(eval $(call KERNRULES,$(dir))))
$(foreach dir,$(LIBRARY_DIRS),$(eval $(call LIBRULES,$(dir))))
$(foreach dir,$(MODULE_DIRS),$(eval $(call MODRULES,$(dir))))

CFLAGS += $(patsubst %,-I%,$(INCLUDES))

DEPS := $(foreach src,$(SOURCES),$(call DEPSRC,$(src)))

ifeq ($(filter clean,$(MAKECMDGOALS)),)
sinclude $(DEPS)
endif

$(OOBJDIR)/%.d: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	$(Q) $(CC) $(CFLAGS) -MM -MT $(call SRCOBJ,$^) -c $^ -o $@
	@ touch $@

$(OOBJDIR)/%.d: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	$(Q) $(CC) -c -MM -MT $(call SRCOBJ,$^) $^ -o $@
	@ touch $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	$(QECHO) CC $(subst $(SRCDIR)/,,$<)
	$(Q) $(CC) $(CFLAGS) -c $< -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	$(QECHO) AS $(subst $(SRCDIR)/,,$<)
	$(Q) $(CC) -c -s $< -o $@

$(BIN).elf: $(KERNOBJS) $(OOBJDIR)/modules-linked.o
	@ mkdir -p $(dir $@)
	$(QECHO) LD $(subst $(OIMGDIR)/,,$@)
	$(Q) $(CC) $(CFLAGS) -Lld -Tsystem.ld -o $@ -Wl,--start-group $^ -Wl,--end-group

$(BIN).elf.strip: $(BIN).elf
	$(Q) $(STRIP) -o $@ $^

$(BIN): $(BIN).elf.strip
	@ mkdir -p $(dir $@)
	$(QECHO) OC $(subst $(OIMGDIR)/,,$@)
	$(Q) $(OBJCOPY) -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 $^ $@

$(OOBJDIR)/modules-linked.o: $(MODOBJS)
	@ mkdir -p $(dir $@)
	$(Q) cat $^ > $(@:.o=.b); echo -n ' ' >> $(@:.o=.b)
	$(Q) $(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@
