
SDK_HOME:=$(sdk_rootfs)/..

ReportReaderBypass.so: ReportReaderBypass.cpp Makefile
	@$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $< \
	        -I$(SDK_HOME)/simulation \
	        -I$(SDK_HOME)/simulation/libs \
	        -I$(SDK_HOME)/simulation/libs/common \
	        -I$(SDK_HOME)/simulation/protos \
	        -I$(SDK_HOME)/simulation/libs/loc \
	        -I$(SDK_HOME)/simulation/services \
	        -I$(SDK_HOME)/simulation/utils \
	        -I$(SDK_HOME)/rootfs/include \
	        -ldl

clean:
	@rm -rf *.so
