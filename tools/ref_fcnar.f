C     ref_fcnar.f -- reference output for fcnar (the lmdif objective). ARMA(1,1)
C     phi=0.5, theta=0.3 on an 8-point working series tsrs. Lprier=F so the
C     diagnostic warning block is skipped (matches the C++ deferral). Two calls:
C     success under exact ML (Lextma=T, residuals scaled by exp(Lndtcv/2/Dnefob)),
C     then an error path (phi=1.05, lckinv=F -> PACFER -> flood with Lrgrsd).
C     getstr/errhdr/prtitr/abend are stubbed (unreachable with Lprier=F).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_fcnar.f \
C         oracle/fortran/{fcnar,upespm,armafl,intgpg,exctma,chkrts,mltpos,ratpos,
C         ratneg,copy,setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,
C         uconv,xpand,euclid,arflt,xprmx,maxlag}.f -o ref_fcnar && ./ref_fcnar
      PROGRAM ref_fcnar
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'series.cmn'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INTEGER na,err,i
      DOUBLE PRECISION a(1092),estprm(2)

C     ---- ARMA(1,1) model shell (coeffs come from estprm via upespm) ----
      Lar=.true.
      Lma=.true.
      Lextma=.true.
      Lprier=.false.
      Nopr=2
      Nestpm=2
      Nspobs=8
      Dnefob=8D0
      Lrgrsd=1D6
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=2
      Mdl(3)=3
      Opr(0)=1
      Opr(1)=2
      Opr(2)=3
      Arimal(1)=1
      Arimal(2)=1
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=1
      Mxmalg=1
      Mxdflg=0
      Tsrs(1)=1D0
      Tsrs(2)=2D0
      Tsrs(3)=-1D0
      Tsrs(4)=0.5D0
      Tsrs(5)=3D0
      Tsrs(6)=-2D0
      Tsrs(7)=1.5D0
      Tsrs(8)=0.25D0

C     ---- success path: exact ML scaling ----
      estprm(1)=0.5D0
      estprm(2)=0.3D0
      Lndtcv=0D0
      na=9
      err=-99
      CALL fcnar(na,2,estprm,a,.false.,.true.,err,.true.)
      WRITE(*,'(A,I4)') 'fc_na ',na
      WRITE(*,'(A,I4)') 'fc_err ',err
      WRITE(*,'(A,ES24.16)') 'fc_ldt ',Lndtcv
      WRITE(*,'(A,ES24.16)') 'fc_a1 ',a(1)
      WRITE(*,'(A,ES24.16)') 'fc_a2 ',a(2)
      WRITE(*,'(A,ES24.16)') 'fc_a9 ',a(9)

C     ---- error path: phi=1.05 non-stationary, lckinv=F -> flood Lrgrsd ----
      estprm(1)=1.05D0
      estprm(2)=0.3D0
      Lndtcv=0D0
      na=9
      err=-99
      CALL fcnar(na,2,estprm,a,.false.,.true.,err,.false.)
      WRITE(*,'(A,I4)') 'ec_na ',na
      WRITE(*,'(A,I4)') 'ec_err ',err
      WRITE(*,'(A,ES24.16)') 'ec_a1 ',a(1)
      WRITE(*,'(A,ES24.16)') 'ec_a9 ',a(9)
      END

C     ---- stubs (Lprier=F makes the diagnostic block unreachable) ----
      SUBROUTINE errhdr
      RETURN
      END
      SUBROUTINE abend
      RETURN
      END
      SUBROUTINE getstr(a,b,c,d,e,f)
      INTEGER b,c,d,f
      CHARACTER a,e
      RETURN
      END
      SUBROUTINE prtitr(a,b,c,d,e,f,g)
      INTEGER b,d,f,g
      DOUBLE PRECISION a,c
      CHARACTER e
      RETURN
      END
