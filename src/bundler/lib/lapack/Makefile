OBJS=dlamch.o \
	dbdsdc.o \
	dbdsqr.o \
	dgebak.o \
	dgebal.o \
	dgebd2.o \
	dgebrd.o \
	dgeev.o \
	dgehd2.o \
	dgehrd.o \
	dgelq2.o \
	dgelqf.o \
	dgelss.o \
	dgelsy.o \
	dgeqp3.o \
	dgeqr2.o \
	dgeqrf.o \
	dgerq2.o \
	dgerqf.o \
	dgesdd.o \
	dgesvd.o \
	dgesv.o \
	dgetf2.o \
	dgetrf.o \
	dgetri.o \
	dgetrs.o \
	dhseqr.o \
	disnan.o \
	dlabad.o \
	dlabrd.o \
	dlacpy.o \
	dladiv.o \
	dlaed6.o \
	dlaexc.o \
	dlahqr.o \
	dlahr2.o \
	dlaic1.o \
	dlaisnan.o \
	dlaln2.o \
	dlamrg.o \
	dlange.o \
	dlanst.o \
	dlanv2.o \
	dlapy2.o \
	dlaqp2.o \
	dlaqps.o \
	dlaqr0.o \
	dlaqr1.o \
	dlaqr2.o \
	dlaqr3.o \
	dlaqr4.o \
	dlaqr5.o \
	dlarfb.o \
	dlarf.o \
	dlarfg.o \
	dlarft.o \
	dlarfx.o \
	dlartg.o \
	dlarzb.o \
	dlarz.o \
	dlarzt.o \
	dlas2.o \
	dlascl.o \
	dlasd0.o \
	dlasd1.o \
	dlasd2.o \
	dlasd3.o \
	dlasd4.o \
	dlasd5.o \
	dlasd6.o \
	dlasd7.o \
	dlasd8.o \
	dlasda.o \
	dlasdq.o \
	dlasdt.o \
	dlaset.o \
	dlasq1.o \
	dlasq2.o \
	dlasq5.o \
	dlasq6.o \
	dlasr.o \
	dlasrt.o \
	dlassq.o \
	dlasv2.o \
	dlaswp.o \
	dlasy2.o \
	dlasyf.o \
	dlatrz.o \
	dlauu2.o \
	dlauum.o \
	dlazq3.o \
	dlazq4.o \
	dorg2r.o \
	dorgbr.o \
	dorghr.o \
	dorgl2.o \
	dorglq.o \
	dorgqr.o \
	dorm2r.o \
	dormbr.o \
	dorml2.o \
	dormlq.o \
	dormqr.o \
	dormr3.o \
	dormrz.o \
	dpotf2.o \
	dpotrf.o \
	dpotri.o \
	dpotrs.o \
	drscl.o \
	dsytf2.o \
	dsytrf.o \
	dsytri.o \
	dsytrs.o \
	dtrevc.o \
	dtrexc.o \
	dtrti2.o \
	dtrtri.o \
	dtrtrs.o \
	dtzrzf.o \
	ieeeck.o \
	ilaenv.o \
	iparmq.o \
	xerbla.o


AR=ar
F77=gcc
FC=gcc
FFLAGS=-O3

all: liblapack.a

dlamch.o: dlamch.f
	$(F77) -c $<

.f.o: 
	$(F77) -c $(FFLAGS) $<

liblapack.a: $(OBJS)
	$(AR) rs $@ $(OBJS)
	cp $@ ../

clean:
	rm -f *.o *~ liblapack.a
