C     ref_fcstxy2.f -- reference output for fcstxy WITH regression (Nb>0), the
C     branch ref_fcstxy skips. Estimates the same ARMA(1,1)+intercept as
C     ref_rgarma2 (Ncxy=2, Nb=1), extends the intercept design into the 6
C     forecast rows (Xy rows 25..30 = [1, *]; the y slot is zeroed by fcstxy),
C     then forecasts 6 steps ahead. Nb=1 with Iregfx=0 -> nb2=1, so the design-
C     uncertainty term X_f(X'X)^-1 X_f' fires (dppsl/yprmy) and Rgvar is nonzero.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_fcstxy2.f \
C         <same object list as ref_fcstxy> -o ref_fcstxy2 && ./ref_fcstxy2
      PROGRAM ref_fcstxy2
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'mdltbl.i'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'series.cmn'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
      INTEGER na,nefobs,i,nfcst,fctori
      LOGICAL lauto
      DOUBLE PRECISION a(1092),ser(24),fcst(6),se(6),rgvar(6)
      DATA ser/0.5D0,-0.3D0,0.8D0,-0.6D0,0.2D0,0.9D0,-0.7D0,0.4D0,
     &         0.1D0,-0.5D0,0.6D0,-0.2D0,0.7D0,-0.8D0,0.3D0,0.5D0,
     &         -0.4D0,0.9D0,-0.1D0,0.6D0,-0.7D0,0.2D0,0.4D0,-0.5D0/

      Nspobs=24
      Ncxy=2
      DO i=1,24
       Xy(2*i-1)=1D0
       Xy(2*i)=ser(i)+2D0
      END DO

      Lar=.true.
      Lma=.true.
      Lextma=.true.
      Lextar=.false.
      Lprier=.false.
      Lhiddn=.false.
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
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=1
      Mxmalg=1
      Mxdflg=0
      Arimap(1)=0.3D0
      Arimap(2)=0.3D0

      Nb=1
      Nintvl=0
      Nextvl=0
      Iregfx=0

      Tol=1D-5
      Nltol0=1D-3
      Nltol=1D-5
      Stepln=0D0

      Issap=0
      Irev=0
      Lfatal=.false.
      Mt1=6
      Mt2=6
      DO i=1,NTBL
       Savtab(i)=.false.
      END DO

      lauto=.false.
      na=0
      nefobs=0
      CALL rgarma(.true.,200,60,.false.,a,na,nefobs,lauto)

C     ---- extend the intercept design into the 6 forecast rows (25..30) ----
      DO i=25,30
       Xy(2*i-1)=1D0
       Xy(2*i)=0D0
      END DO

      fctori=Nspobs
      nfcst=6
      CALL fcstxy(fctori,nfcst,fcst,se,rgvar)

      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'fc_fcst',i,' ',fcst(i)
      END DO
      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'fc_se',i,' ',se(i)
      END DO
      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'fc_rgvar',i,' ',rgvar(i)
      END DO
      END

      SUBROUTINE errhdr
      RETURN
      END
      SUBROUTINE abend
      RETURN
      END
      SUBROUTINE writln(a,b,c,d)
      CHARACTER a
      INTEGER b,c
      LOGICAL d
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
      SUBROUTINE savitr(a,b,c,d,e,f)
      LOGICAL a
      INTEGER b,c,f
      DOUBLE PRECISION d,e
      RETURN
      END
