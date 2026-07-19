C     ref_outlier.f -- golden values for the outlier-identification leaves:
C     shlsrt, medabs, makotl, dppdi, ttest. Small fixed inputs; prints results
C     for the C++ unit tests to match.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_outlier.f \
C         oracle/fortran/{shlsrt,medabs,makotl,dppdi,ttest,xprmx,dppfa,dpmpar,
C         ddot,daxpy,dscal,dpeq,setdp,setint,copy}.f -o ref_outlier && ./ref_outlier
      PROGRAM ref_outlier
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INTEGER i,notlr,ltest(POTLR),mxcol(POTLR),nspobs,ncxy
      DOUBLE PRECISION s(8),med,otlvar(30),xy(12),xpx(3),det(2)
      DOUBLE PRECISION chlxpx(3),propt(POTLR)
      LOGICAL snglr(POTLR)
      INTEGER info
c     ---- medabs (even n=8) ----
      s(1)=3D0
      s(2)=-1D0
      s(3)=4D0
      s(4)=-1D0
      s(5)=5D0
      s(6)=-9D0
      s(7)=2D0
      s(8)=-6D0
      CALL medabs(s,8,med)
      WRITE(*,'(A,F20.14)') 'medabs8= ',med
      CALL medabs(s,5,med)
      WRITE(*,'(A,F20.14)') 'medabs5= ',med
c     ---- makotl AO+LS+TC, t0=3, nr=5, tcalfa=0.7 ----
      ltest(AO)=1
      ltest(LS)=1
      ltest(TC)=1
      CALL makotl(3,5,ltest,otlvar,notlr,0.7D0,12)
      WRITE(*,'(A,I2)') 'makotl_notlr= ',notlr
      WRITE(*,'(A,15F7.3)') 'makotl_all= ',(otlvar(i),i=1,15)
c     ---- dppdi on packed [4,2,3] (upper 2x2) ----
      xpx(1)=4D0
      xpx(2)=2D0
      xpx(3)=3D0
      CALL dppfa(xpx,2,info)
      CALL dppdi(xpx,2,det,11)
      WRITE(*,'(A,3F20.14)') 'dppdi_inv= ',xpx(1),xpx(2),xpx(3)
      WRITE(*,'(A,2F20.14)') 'dppdi_det= ',det(1),det(2)
c     ---- ttest: X=const(6), y=[2,3,1,8,4,5]; AO at t0=3 ----
      nspobs=6
      ncxy=2
      xy(1)=1D0
      xy(2)=2D0
      xy(3)=1D0
      xy(4)=3D0
      xy(5)=1D0
      xy(6)=1D0
      xy(7)=1D0
      xy(8)=8D0
      xy(9)=1D0
      xy(10)=4D0
      xy(11)=1D0
      xy(12)=5D0
      CALL xprmx(xy,nspobs,ncxy,ncxy,chlxpx)
      CALL dppfa(chlxpx,ncxy,info)
      ltest(AO)=1
      ltest(LS)=0
      ltest(TC)=0
      CALL makotl(3,nspobs,ltest,otlvar,notlr,0.7D0,12)
      CALL ttest(xy,nspobs,ncxy,chlxpx,otlvar,ltest,mxcol,propt,snglr)
      WRITE(*,'(A,F20.14)') 'ttest_AO= ',propt(AO)
c     ---- ttest AO+LS+TC at t0=4 ----
      ltest(AO)=1
      ltest(LS)=1
      ltest(TC)=1
      CALL makotl(4,nspobs,ltest,otlvar,notlr,0.7D0,12)
      CALL ttest(xy,nspobs,ncxy,chlxpx,otlvar,ltest,mxcol,propt,snglr)
      WRITE(*,'(A,3F20.14)') 'ttest3= ',propt(AO),propt(LS),propt(TC)
      WRITE(*,'(A,3I3)') 'ttest3_mxcol= ',mxcol(1),mxcol(2),mxcol(3)
      END
C     ---- stubs (error paths not exercised) ----
      SUBROUTINE abend
      RETURN
      END
      SUBROUTINE errhdr
      RETURN
      END
