C     ref_armastat.f -- reference output for xrlkhd (AICC) and armats (ARMA
C     t-statistics), post-estimation stats that consume rgarma's output state.
C     Reuses the ref_rgarma.f Nb=0 ARMA(1,1) case: run rgarma to convergence,
C     then xrlkhd(Aicc, Nxcld=0) and armats(Tval). Lprier/Lprtit=F.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_armastat.f \
C         oracle/fortran/{rgarma,strtvl,setmdl,upespm,roots,revrse,rpoly,fxshfr,\
C         quadit,realit,calcsc,nextk,newest,quadsd,quad,chkrt2,olsreg,resid,\
C         yprmy,maxvec,stpitr,fcnar,armafl,intgpg,exctma,chkrts,mltpos,ratpos,\
C         ratneg,arflt,maxlag,lmdif,fdjac2,qrfac,qrsolv,lmpar,enorm,covar,copy,\
C         dcopy,setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,uconv,\
C         xpand,euclid,xprmx,xrlkhd,armats}.f -o ref_armastat && ./ref_armastat
      PROGRAM ref_armastat
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
      INTEGER na,nefobs,i
      LOGICAL lauto
      DOUBLE PRECISION a(1092),aicc,tval(2)

      Nspobs=24
      Ncxy=1
      Xy(1)=0.5D0
      Xy(2)=-0.3D0
      Xy(3)=0.8D0
      Xy(4)=-0.6D0
      Xy(5)=0.2D0
      Xy(6)=0.9D0
      Xy(7)=-0.7D0
      Xy(8)=0.4D0
      Xy(9)=0.1D0
      Xy(10)=-0.5D0
      Xy(11)=0.6D0
      Xy(12)=-0.2D0
      Xy(13)=0.7D0
      Xy(14)=-0.8D0
      Xy(15)=0.3D0
      Xy(16)=0.5D0
      Xy(17)=-0.4D0
      Xy(18)=0.9D0
      Xy(19)=-0.1D0
      Xy(20)=0.6D0
      Xy(21)=-0.7D0
      Xy(22)=0.2D0
      Xy(23)=0.4D0
      Xy(24)=-0.5D0

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

      Nb=0
      Nintvl=0
      Nextvl=0
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

      aicc=-99D0
      CALL xrlkhd(aicc,0)
      tval(1)=-99D0
      tval(2)=-99D0
      CALL armats(tval)

      WRITE(*,'(A,ES24.16)') 'as_aicc ',aicc
      WRITE(*,'(A,ES24.16)') 'as_tphi ',tval(1)
      WRITE(*,'(A,ES24.16)') 'as_ttheta ',tval(2)
      END

C     ---- stubs (unreachable with Lprier=F, Lprtit=F, Armaer!=PACSER) ----
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
