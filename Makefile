CORPUS_DIR = ./fuzz_input/$(TGT)
REPORT_DIR = ./fuzz_output/$(TGT)

fuzz: prepare_dirs
	../honggfuzz/honggfuzz -Q \
		-W $(REPORT_DIR) \
		--input $(CORPUS_DIR) \
		-- ./build/fuzzinger/src/$(TGT)/$(TGT)_fuzz

prepare_dirs:
	mkdir -p $(CORPUS_DIR) $(REPORT_DIR)
