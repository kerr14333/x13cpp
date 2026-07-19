C     ref_fcstxy3.f -- reference output for fcstxy on a DIFFERENCED model (0 1 1),
C     the branch ref_fcstxy/ref_fcstxy2 skip (both mxdfar=0). Nonseasonal diff
C     (1-B, coef 1 fixed) + MA(1): Mxdflg=1, Mxarlg=0 -> mxdfar=1, so tfcst is
C     seeded from the last row of Xy and the ndltar-offset recursion runs. Series
C     is an I(1) cumulative sum so the differenced fit is well-behaved. Nb=0.
C     Golden dump: Fcst(1..6), Se(1..6), Rgvar(1..6) (Rgvar=0, no regression).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_fcstxy3.f \
C         <same object list as ref_fcstxy> -o ref_fcstxy3 && ./ref_fcstxy3
      PROGRAM ref_fcstxy3
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
      DOUBLE PRECISION a(1092),ser(24),fcst(6),se(6),rgvar(6),acc
      DATA ser/0.5D0,-0.3D0,0.8D0,-0.6D0,0.2D0,0.9D0,-0.7D0,0.4D0,
     &         0.1D0,-0.5D0,0.6D0,-0.2D0,0.7D0,-0.8D0,0.3D0,0.5D0,
     &         -0.4D0,0.9D0,-0.1D0,0.6D0,-0.7D0,0.2D0,0.4D0,-0.5D0/

C     ---- I(1) series: running cumulative sum (base 10) of ser ----
      Nspobs=24
      Ncxy=1
      acc=10D0
      DO i=1,24
       acc=acc+ser(i)
       Xy(i)=acc
      END DO

C     ---- (0 1 1) model shell: DIFF op (lag 1, coef 1 fixed) + MA op (lag 1) ----
      Lar=.false.
      Lma=.true.
      Lextma=.true.
      Lextar=.true.
      Lprier=.false.
      Lhiddn=.false.
      Nopr=2
      Mdl(0)=1
      Mdl(1)=2
      Mdl(2)=2
      Mdl(3)=3
      Opr(0)=1
      Opr(1)=2
      Opr(2)=3
      Arimal(1)=1
      Arimal(2)=1
      Arimaf(1)=.true.
      Arimaf(2)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=0
      Mxmalg=1
      Mxdflg=1
      Arimap(1)=1.0D0
      Arimap(2)=0.3D0

      Nb=0
      Nintvl=1
      Nextvl=1
      Iregfx=0

      Tol=1D-5
      Nltol0=1D-5
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

      fctori=Nspobs
      nfcst=6
      CALL fcstxy(fctori,nfcst,fcst,se,rgvar)

      WRITE(*,'(A,L2)') 'rg_convrg ',Convrg
      WRITE(*,'(A,ES24.16)') 'rg_theta ',Arimap(2)
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
