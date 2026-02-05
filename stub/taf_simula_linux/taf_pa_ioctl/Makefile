
taf_pa_ioctl.so: taf_pa_ioctl.c Makefile
	@$(CXX) $(CXXFLAGS) -shared -fPIC \
		-I$(LEGATO_ROOT)/framework/include \
		-o $@ $< -ldl

clean:
	@rm -rf *.so
