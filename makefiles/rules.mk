$(foreach mod, $(MODULES), $(eval $(call SCANMOD,$(mod))))
$(foreach src, $(SOURCES) $(ASSEMBLY), $(eval $(call DEPSRC,$(src))))

ifeq ($(filter clean,$(MAKECMDGOALS)),)
sinclude $(DEPS)
endif

$(BIN).elf: $(OBJECTS) $(OOBJDIR)/modules-linked.o
	@ mkdir -p $(dir $@)
	$(QECHO) LD $(subst $(OIMGDIR)/,,$@)
	$(Q) $(CC) $(CFLAGS) -Lld -Tsystem.ld -o $@ -Wl,--start-group $^ -Wl,--end-group

$(BIN).elf.strip: $(BIN).elf
	$(Q) $(STRIP) -o $@ $^

$(BIN): $(BIN).elf.strip
	@ mkdir -p $(dir $@)
	$(QECHO) OC $(subst $(OIMGDIR)/,,$@)
	$(Q) $(OBJCOPY) -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 $^ $@

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
	$(QECHO) CC $<
	$(Q) $(CC) $(CFLAGS) -c $< -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	$(QECHO) AS $<
	$(Q) $(CC) -c -s $< -o $@

$(OMODDIR)/%.o: $(OOBJDIR)/mod_%.o
	@ mkdir -p $(dir $@)
	$(QECHO) MODLD $(@:$(OMODDIR)/%.o=%)
	$(Q) $(CC) $(CFLAGS) -shared -Lld -Tmodule.ld -r -o $@ -s $^

$(OOBJDIR)/modules-linked.o: $(MODOBJS)
	@ mkdir -p $(dir $@)
	$(Q) cat $^ > $(@:.o=.b)
	$(Q) $(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@
