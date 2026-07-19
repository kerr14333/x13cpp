C     ref_armafl.f -- reference output for intgpg + exctma (exact MA filter
C     helpers). Builds a minimal pure-MA(2) model in the model/mdldat commons
C     (theta = 0.3 B - 0.2 B^2), runs intgpg then exctma on a length-5 series.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_armafl.f \
C         oracle/fortran/intgpg.f oracle/fortran/exctma.f oracle/fortran/ratpos.f \
C         oracle/fortran/ratneg.f oracle/fortran/copy.f oracle/fortran/setdp.f \
C         oracle/fortran/scrmlt.f oracle/fortran/dppfa.f oracle/fortran/dsolve.f \
C         oracle/fortran/logdet.f oracle/fortran/dpmpar.f -o ref_armafl && ./ref_armafl
      PROGRAM ref_armafl
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INTEGER nextma,info,nelta,i
      DOUBLE PRECISION a(200)

C     ---- minimal pure MA(2) model: one MA operator, lags 1 & 2 ----
      Lma=.true.
      Lar=.false.
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=1
      Mdl(3)=2
      Opr(0)=1
      Opr(1)=3
      Arimal(1)=1
      Arimal(2)=2
      Arimap(1)=0.3D0
      Arimap(2)=-0.2D0
      Oprfac(1)=1
      Mxmalg=2
      Lndtcv=-99D0

C     ---- intgpg: build & factor G'G, set Lndtcv ----
      nextma=6
      info=-99
      CALL intgpg(nextma,info)
      WRITE(*,'(A,I4)')      'gpg_info ',info
      WRITE(*,'(A,ES24.16)') 'gpg_c1 ',Chlgpg(1)
      WRITE(*,'(A,ES24.16)') 'gpg_c2 ',Chlgpg(2)
      WRITE(*,'(A,ES24.16)') 'gpg_c3 ',Chlgpg(3)
      WRITE(*,'(A,ES24.16)') 'gpg_ldt ',Lndtcv

C     ---- exctma: exact MA filter a length-5 series (nc=1) ----
      a(1)=1.0D0
      a(2)=2.0D0
      a(3)=-1.0D0
      a(4)=0.5D0
      a(5)=3.0D0
      nelta=5
      CALL exctma(1,a,nelta,200)
      WRITE(*,'(A,I4)') 'exc_nelta ',nelta
      WRITE(*,'(A,I4)') 'exc_nopr ',Nopr
      DO i=1,7
       WRITE(*,'(A,I1,A,ES24.16)') 'exc_a',i,' ',a(i)
      END DO
      END
