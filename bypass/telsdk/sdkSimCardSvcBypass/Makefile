
SDK_HOME:=$(sdk_rootfs)/..

SRCS := PhoneFactoryImplStubBypass.cpp SimProfileManagerBypass.cpp MultiSimManagerImplBypass.cpp

PhoneFactoryImplStubBypass.so: $(SRCS) Makefile
	@$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $(SRCS) \
	        -I$(SDK_HOME)/simulation \
	        -I$(SDK_HOME)/simulation/libs \
	        -I$(SDK_HOME)/simulation/libs/common \
	        -I$(SDK_HOME)/simulation/protos \
	        -I$(SDK_HOME)/simulation/libs/tel \
	        -I$(SDK_HOME)/simulation/services \
	        -I$(SDK_HOME)/simulation/utils \
	        -I$(SDK_HOME)/rootfs/include \
	        -ldl

clean:
	@rm -rf *.so
