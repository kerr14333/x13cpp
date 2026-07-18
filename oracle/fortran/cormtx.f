C     Last change:  BCM   2 Jun 1998   11:33 am
      SUBROUTINE cormtx(Xpxinv,Regidx)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Calculates and print the correlation matrix from (X'X)^-1
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER str*(PCOLCR)
      INTEGER icol,idiag,ielt,irow,nchr,Regidx,tcol,jcol,i1,i2,j
      DOUBLE PRECISION scale,Xpxinv
      DIMENSION Xpxinv(Nb*Ncxy/2),Regidx(PB),tcol(PB)
c-----------------------------------------------------------------------
      jcol=Nb
      j=1
      IF(Iregfx.eq.2)THEN
       DO icol=1,Nb
        IF(Regidx(icol).ne.NOTSET)THEN
         tcol(j)=icol
         j=j+1
        ElSE
         jcol=jcol-1
        END IF
       END DO
       WRITE(Mt1,1010)(tcol(icol),icol=1,jcol)
      ELSE
       WRITE(Mt1,1010)(icol,icol=1,jcol)
      END IF
 1010 FORMAT(/,' Correlation matrix',/,'  Variable',(:t20,10I6))
      WRITE(Mt1,1020)('-',icol=1,17+6*min(jcol,10))
 1020 FORMAT('  ',(78a))
c-----------------------------------------------------------------------
      idiag=0
      DO icol=1,Nb
       IF(Regidx(icol).ne.NOTSET)THEN
        idiag=idiag+Regidx(icol)
        scale=sqrt(Xpxinv(idiag))
c-----------------------------------------------------------------------
        DO ielt=idiag-Regidx(icol)+1,idiag
c         IF(Regidx(ielt).ne.NOTSET)THEN
c          jcol=Regidx(ielt)
          Xpxinv(ielt)=Xpxinv(ielt)/scale
c         END IF
        END DO
c-----------------------------------------------------------------------
        ielt=idiag
        DO irow=icol,Nb
         IF(Regidx(irow).ne.NOTSET)THEN
          Xpxinv(ielt)=Xpxinv(ielt)/scale
          ielt=ielt+Regidx(irow)
         END IF
        END DO
c-----------------------------------------------------------------------
        CALL getstr(Colttl,Colptr,Ncoltl,icol,str,nchr)
        IF(Lfatal)RETURN
        i1=idiag-Regidx(icol)+1
        i2=i1+9
        IF(i2.gt.idiag)i2=idiag
        WRITE(Mt1,1030)str(1:nchr),(Xpxinv(ielt),ielt=i1,i2)
 1030   FORMAT('  ',a,t20,10F6.2)
        DO WHILE(i2.lt.idiag)
         i1=i2+1
         i2=i1+9
         IF(i2.gt.idiag)i2=idiag
         WRITE(Mt1,1040)(Xpxinv(ielt),ielt=i1,i2)
 1040    FORMAT(t20,10F6.2)
        END DO
       END IF
      END DO
c-----------------------------------------------------------------------
      RETURN
c-----------------------------------------------------------------------
      END
