C     ref_armafl_reuse.f -- reference output for the Linit=F reuse path (A7).
C     Init an ARMA(1,1) on Nr=8 (Linit=T), then re-filter a NEW Nr=12 series with
C     Linit=F, Lckrts=F: SAVEd nextma is recomputed for the new Nr while
C     Chlgpg/Chlvwp/Matd keep the Nr=8 factorization, and the ddot correction
C     reads the zero Matd tail (elements past what the Nr=8 D-build wrote). Matd
C     is explicitly zeroed first so the run matches a fresh C++ X13Context.
C     This is exactly what forecasting (fcstxy) and outlier detection (idotlr) do.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_armafl_reuse.f \
C         oracle/fortran/{armafl,intgpg,exctma,chkrts,mltpos,ratpos,ratneg,copy,
C         setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,uconv,xpand,
C         euclid,arflt,xprmx,maxlag}.f -o ref_armafl_reuse && ./ref_armafl_reuse
      PROGRAM ref_armafl_reuse
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INTEGER na,info,i
      DOUBLE PRECISION mata(200)

      DO i=1,64
       Matd(i)=0D0
      END DO
      Lar=.true.
      Lma=.true.
      Nopr=2
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=2
      Mdl(3)=3
      Opr(0)=1
      Opr(1)=2
      Opr(2)=3
      Arimal(1)=1
      Arimal(2)=1
      Arimap(1)=0.5D0
      Arimap(2)=0.3D0
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=1
      Mxmalg=1
      Mxdflg=0
      Lndtcv=0D0

C     ---- call 1: Linit=T on Nr=8 ----
      DO i=1,8
       mata(i)=DBLE(MOD(7*i,13))-6D0+0.25D0*DBLE(MOD(3*i,8))
      END DO
      info=-99
      na=-99
      CALL armafl(8,1,.true.,.true.,mata,na,200,info)
      WRITE(*,'(A,I4)')      'r1_na ',na
      WRITE(*,'(A,ES24.16)') 'r1_ldt ',Lndtcv

C     ---- call 2: Linit=F, Lckrts=F on a NEW Nr=12 series (reuse state) ----
      DO i=1,12
       mata(i)=DBLE(MOD(5*i,11))-5D0+0.5D0*DBLE(MOD(2*i,7))
      END DO
      info=-99
      na=-99
      CALL armafl(12,1,.false.,.false.,mata,na,200,info)
      WRITE(*,'(A,I4)')      'r2_info ',info
      WRITE(*,'(A,I4)')      'r2_na ',na
      WRITE(*,'(A,ES24.16)') 'r2_ldt ',Lndtcv
      WRITE(*,'(A,ES24.16)') 'r2_a1 ',mata(1)
      WRITE(*,'(A,ES24.16)') 'r2_a7 ',mata(7)
      WRITE(*,'(A,ES24.16)') 'r2_alast ',mata(na)
      END
