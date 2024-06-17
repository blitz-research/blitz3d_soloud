include ../../Makefile.cfg

INCLUDE = -I../
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)

TARGET  = freeimage
VER_MAJOR = 2
VER_MINOR = 0.0
MODULES = BitmapAccess.o Conversion.o Conversion16_555.o Conversion16_565.o \
	Conversion24.o Conversion32.o Conversion8.o FreeImage.o GetType.o Plugin.o \
	PluginBMP.o PluginICO.o PluginJPEG.o PluginKOALA.o PluginMNG.o PluginPCD.o \
	PluginPCX.o PluginPNG.o PluginPNM.o PluginRAS.o PluginTARGA.o PluginTIFF.o \
        PluginWBMP.o PluginLBM.o OldLoadFunctions.o NNQuantizer.o WuQuantizer.o Combine.o

STATICLIB = lib$(TARGET).a
SHAREDLIB = lib$(TARGET)-$(VER_MAJOR).$(VER_MINOR).so
LIBNAME = lib$(TARGET).so.$(VER_MAJOR)

all: default

default: $(STATICLIB) $(SHAREDLIB)

.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.o:
	$(CPP) $(CFLAGS) -c $<

$(STATICLIB): $(MODULES)
	ar r $@ $?
 
$(SHAREDLIB): $(MODULES)
	$(CC) -s -shared -Wl,-soname,$(LIBNAME) -o $@ $? $(LIBRARIES) 
	ln -sf $(SHAREDLIB) $(LIBNAME)

install: $(STATICLIB) $(SHAREDLIB)
	install -m 644 -o root -g root $(STATICLIB) $(INSTALLDIR)
	install -m 755 -o root -g root $(SHAREDLIB) $(INSTALLDIR)
	ln -sf $(SHAREDLIB) $(INSTALLDIR)/$(LIBNAME)
	ldconfig

clean:
	rm -f *.o core d2utmp* $(STATICLIB) $(SHAREDLIB) $(LIBNAME)
