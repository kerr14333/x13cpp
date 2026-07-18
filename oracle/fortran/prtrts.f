C     Last change:  BCM  25 Jun 1998   10:10 am
      SUBROUTINE prtrts(Lprt,Lsav,Lsvlg,Ldiag)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Prints out the roots of phi(B)=0 and theta(B)=0; each root has
c four components: Real, Imaginary, Module, and Frequency
c-----------------------------------------------------------------------
c Name   Type Description
c-----------------------------------------------------------------------
c dotln   c  Local pgrpcr character dotted line under the model title
c degree  i  Maximum lag of phi(B) or theta(B)
c degp1   i  degree + 1
c coeff   d  Coefficients of phi(B) or theta(B) in order of increasing
c             powers
c rcoef   d  Coefficients of phi(B) or theta(B) in order of decreasing
c             powers
c zeror   d  Real part of the roots
c zeroi   d  Imaginary part of the roots
c-----------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c-----------------------------------------------------------------------
      INCLUDE 'cchars.i'
      INCLUDE 'mdltbl.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      CHARACTER lower1*(1),lower2*(4),dotln*(POPRCR+1),tmpttl*(POPRCR),
     &          outstr*(100)
      LOGICAL allinv,Lprt,Lsav,Lsvlg,Ldiag,fcnok
      INTEGER i,i2,k,beglag,begopr,endlag,endopr,factor,iflt,ilag,iopr,
     &        ntmpcr,degree,spchr,fh,ipos
      DOUBLE PRECISION coeff(PORDER+1),zeror(PORDER),zeroi(PORDER),
     &                 zerom(PORDER),zerof(PORDER)
      DATA dotln/
     &   '  -----------------------------------------------------------'
     &   /
c-----------------------------------------------------------------------
c     Print out the roots of phi(B)=0 and theta(B)=0 with AR part first
c-----------------------------------------------------------------------
      begopr=Mdl(AR-1)
      beglag=Opr(begopr-1)
      endopr=Mdl(MA)-1
c     ------------------------------------------------------------------
c     Nov 2005 BCM - add statement to avoid printing out root table
c                    when no ARMA operators are in the model
      IF(endopr.gt.0.and.begopr.le.endopr)THEN
       endlag=Opr(endopr)-1
c     ------------------------------------------------------------------
       IF(Lprt)WRITE(Mt1,1010)Mdlttl(1:Nmdlcr),dotln
       IF(Lsvlg)WRITE(Ng,1010)Mdlttl(1:Nmdlcr),dotln
 1010  FORMAT(/,' Roots of ',a,/,'  Root',t25,'Real',t31,'Imaginary',
     &        t44,'Modulus',t53,'Frequency',/,a)
       IF(Lsav)THEN
        CALL opnfil(T,F,LESTRT,fh,fcnok)
        IF(.not.fcnok)THEN
         CALL abend
         RETURN
        END IF
        WRITE(fh,1011)TABCHR,TABCHR,TABCHR,TABCHR,TABCHR,TABCHR,
     &                TABCHR,TABCHR,TABCHR,TABCHR,TABCHR,TABCHR
 1011   FORMAT('Operator',a,'Factor',a,'Root',a,'Real',a,'Imaginary',a,
     &         'Modulus',a,'Frequency',/,'--------',a,'------',a,
     &         '----',a,'----',a,'---------',a,'-------',a,'---------')
       END IF
c     ------------------------------------------------------------------
       DO iflt=AR,MA
        begopr=Mdl(iflt-1)
        endopr=Mdl(iflt)-1
c     ------------------------------------------------------------------
        DO iopr=begopr,endopr
         beglag=Opr(iopr-1)
         endlag=Opr(iopr)-1
c     ------------------------------------------------------------------
         CALL getstr(Oprttl,Oprptr,Noprtl,iopr,tmpttl,ntmpcr)
         IF(Lfatal)RETURN
         IF(Lprt)WRITE(Mt1,1020)tmpttl(1:ntmpcr)
         IF(Lsvlg)WRITE(Ng,1020)tmpttl(1:ntmpcr)
         IF(Lsav.or.Ldiag)THEN
          DO spchr=ntmpcr,1,-1
           IF(tmpttl(spchr:spchr).eq.' ')GO TO 10
          END DO
          spchr=1
         END IF
 1020    FORMAT('  ',a,t35)
c     ------------------------------------------------------------------
   10    factor=Oprfac(iopr)
         degree=Arimal(endlag)/factor
         coeff(1)=-1.0D0
         CALL setdp(0D0,degree,coeff(2))
c         DO i=2,degree+1
c          coeff(i)=0.d0
c         END DO
c     ------------------------------------------------------------------
         DO ilag=beglag,endlag
          coeff(Arimal(ilag)/factor+1)=Arimap(ilag)
         END DO
         CALL roots(coeff,degree,allinv,zeror,zeroi,zerom,zerof)
         IF(Lfatal)RETURN
c     ------------------------------------------------------------------
         DO i=1,degree
          IF(Lprt)WRITE(Mt1,1030)i,zeror(i),zeroi(i),zerom(i),zerof(i)
          IF(Lsvlg)WRITE(Ng,1030)i,zeror(i),zeroi(i),zerom(i),zerof(i)
 1030     FORMAT('   Root',i3,t18,4F11.4)
          IF(Lsav.or.Ldiag)THEN
           ipos=1
           CALL dtoc(zeror(i),outstr,ipos)
           IF(Lfatal)RETURN
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           CALL dtoc(zeroi(i),outstr,ipos)
           IF(Lfatal)RETURN
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           CALL dtoc(zerom(i),outstr,ipos)
           IF(Lfatal)RETURN
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           CALL dtoc(zerof(i),outstr,ipos)
           IF(Lfatal)RETURN
           IF(Lsav)THEN
            WRITE(fh,1031)tmpttl(spchr+1:ntmpcr),TABCHR,
     &                    tmpttl(1:spchr-1),TABCHR,i,TABCHR,
     &                    outstr(1:(ipos-1))
           END IF
           IF(Ldiag)THEN
            lower1=CHAR(ICHAR(tmpttl(1:1))+32)
            DO k=spchr+1,ntmpcr
             i2=k-spchr
             lower2(i2:i2)=CHAR(ICHAR(tmpttl(k:k))+32)
            END DO
            WRITE(Nform,1031)'roots.'//lower2(1:i2),'.',
     &                       lower1//tmpttl(2:spchr-1),'.',i,': ',
     &                       outstr(1:(ipos-1))
           END IF
 1031      FORMAT(a,a,a,a,i2.2,a,a)
          END IF
         END DO
        END DO
       END DO
       IF(Lprt)WRITE(Mt1,1040)dotln
       IF(Lsvlg)WRITE(Ng,1041)dotln
       IF(Lsav)CALL fclose(fh)
      END IF
c     ------------------------------------------------------------------
 1040 FORMAT(a)
 1041 FORMAT(a,/)
      RETURN
      END
