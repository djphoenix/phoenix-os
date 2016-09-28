$(foreach mod, $(MODULES), $(eval $(call SCANMOD,$(mod))))
$(foreach src, $(SOURCES) $(ASSEMBLY), $(eval $(call DEPSRC,$(src))))

sinclude $(DEPS)

$(BIN).elf: $(OBJECTS) $(OOBJDIR)/modules-linked.o
	@ mkdir -p $(dir $@)
	@ echo LD $(subst $(OIMGDIR)/,,$@)
	@ $(CC) $(CFLAGS) -Tld.script -o $@ -Wl,--start-group $^ -Wl,--end-group

$(BIN).elf.strip: $(BIN).elf
	@ $(STRIP) -o $@ $^

$(BIN): $(BIN).elf.strip
	@ mkdir -p $(dir $@)
	@ echo OC $(subst $(OIMGDIR)/,,$@)
	@ $(OBJCOPY) -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 $^ $@

$(OOBJDIR)/%.d: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) -MM -MT $(call SRCOBJ,$^) -c $^ -o $@

$(OOBJDIR)/%.d: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	@ $(CC) -c -MM -MT $(call SRCOBJ,$^) $^ -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	@ echo CC $<
	@ $(CC) $(CFLAGS) -c $< -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	@ echo AS $<
	@ $(CC) -c -s $< -o $@

$(OMODDIR)/%.o: $(OOBJDIR)/mod_%.o
	@ mkdir -p $(dir $@)
	@ echo MODLD $(@:$(OMODDIR)/%.o=%)
	@ $(CC) $(CFLAGS) -Tld-mod.script -r -o $@ -s $^

$(OOBJDIR)/modules-linked.o: $(MODOBJS)
	@ mkdir -p $(dir $@)
	@ cat $^ > $(@:.o=.b)
	@ $(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@
