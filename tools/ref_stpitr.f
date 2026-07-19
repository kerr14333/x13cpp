C     ref_stpitr.f -- reference output for stpitr (IGLS step/convergence test),
C     a LOGICAL FUNCTION that SAVEs oldobj between calls. Multi-call sequence to
C     exercise the oldobj carry-over plus the three error branches:
C       call 1 (Iter=1)                -> ELSE: oldobj:=0 then oldobj:=Objfcn.
C       call 2 (Iter=2, obj 10->5)     -> ratio uses oldobj=10 (carried).
C       call 3 (Iter=3, obj 5->5.00002)-> |ratio|<Devtol => converged (stpitr=F).
C                                          Proves the carry: needs oldobj=5 (not 0).
C       call 4 (Nliter>=Mxiter)        -> PMXIER, stpitr=F, Convrg=F.
C       call 5 (Devtol/2 < mprec)      -> PCNTER, stpitr=F, Convrg=F.
C       call 6 (Objfcn<mprec, nonzero) -> PDVTER, stpitr=F.
C     Lprier=.false. throughout so the deviance-increase WRITE block is skipped
C     (matches the C++ deferral); errhdr/abend are stubbed (unreachable here).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_stpitr.f \
C         oracle/fortran/{stpitr,dpmpar,dpeq}.f -o ref_stpitr && ./ref_stpitr
      PROGRAM ref_stpitr
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      LOGICAL stpitr,res,convrg
      EXTERNAL stpitr
      INTEGER armaer

C     ---- call 1: first iteration, sets oldobj (via ELSE then oldobj:=Objfcn) --
      armaer=0
      convrg=.false.
      res=stpitr(.false.,10D0,1D-5,1,1,100,convrg,armaer,.false.)
      CALL prcall(1,res,convrg,armaer)

C     ---- call 2: Iter=2, deviance decreased 10->5, ratio uses carried oldobj --
      armaer=0
      convrg=.false.
      res=stpitr(.false.,5D0,1D-5,2,2,100,convrg,armaer,.false.)
      CALL prcall(2,res,convrg,armaer)

C     ---- call 3: Iter=3, 5->5.00002, |ratio|<Devtol -> converged (needs carry)-
      armaer=0
      convrg=.false.
      res=stpitr(.false.,5.00002D0,1D-5,3,3,100,convrg,armaer,.false.)
      CALL prcall(3,res,convrg,armaer)

C     ---- call 4: Nliter>=Mxiter -> PMXIER ----
      armaer=0
      convrg=.false.
      res=stpitr(.false.,5D0,1D-5,4,100,100,convrg,armaer,.false.)
      CALL prcall(4,res,convrg,armaer)

C     ---- call 5: Devtol/2 < mprec -> PCNTER ----
      armaer=0
      convrg=.false.
      res=stpitr(.false.,5D0,1D-18,5,5,100,convrg,armaer,.false.)
      CALL prcall(5,res,convrg,armaer)

C     ---- call 6: Objfcn<mprec but nonzero -> PDVTER ----
      armaer=0
      convrg=.false.
      res=stpitr(.false.,1D-18,1D-5,6,6,100,convrg,armaer,.false.)
      CALL prcall(6,res,convrg,armaer)
      END

C     ---- labeled dump of one call's outputs (res/convrg as 0/1) ----
      SUBROUTINE prcall(k,res,convrg,armaer)
      IMPLICIT NONE
      INTEGER k,armaer,ires,icnv
      LOGICAL res,convrg
      ires=0
      IF(res)ires=1
      icnv=0
      IF(convrg)icnv=1
      WRITE(*,'(A,I1,A,I2,A,I2,A,I3)')
     &   'st',k,' res ',ires,' cnv ',icnv,' aer ',armaer
      RETURN
      END

C     ---- stubs (Lprier=.false. makes the WRITE block unreachable) ----
      SUBROUTINE errhdr
      RETURN
      END
      SUBROUTINE abend
      RETURN
      END
