C     Last change:  BCM  15 Oct 1998    1:35 pm
      SUBROUTINE ssrng(X,Cpobs,Iagr,Lrange,Ncol,Muladd)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c  *****  performs range analysis of the seasonal factors.  calculates
c  *****  monthly means of the seasonal factors for each span and
c  *****  for all spans (xavg), as well as the maximum percentage
c  *****  difference for the monthly means (xmpd).  computes values of
c  *****  the range (xran) for monthly means
c  *****  and seasonal factors for each span and for all spans.  prints
c  *****  out results.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssap.cmn'
      INCLUDE 'notset.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'ssptbl.i'
c-----------------------------------------------------------------------
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.)
c-----------------------------------------------------------------------
      LOGICAL Lrange
      CHARACTER alab*(6),xcm*(3),clab*(6),Cpobs*(9),fmt1*(48),fmt2*(32),
     &          fmt3*(48),numf*(1),sflab*(36)
      INTEGER i,Iagr,j,jsea,k,k1,Kountr,l,lagr,Muladd,narg,Ncol,nsflab
      DOUBLE PRECISION X,xavg,xmn,xmnx,xmpd,xmx,xran
      DIMENSION X(MXLEN,MXCOL),xavg((MXCOL+1),PSP),xmnx((MXCOL+1),2),
     &          xran(MXCOL+1),xmpd(PSP),Kountr(MXCOL,PSP),clab(2,6),
     &          Cpobs(20),numf(3),xcm((MXCOL+1),PSP)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      COMMON /kcom  / Kountr
c-----------------------------------------------------------------------
      DATA(numf(i),i=1,3)/'2','3','4'/
      DATA(clab(1,i),i=1,6)/'      ','      ','      ','      ',
     &                      ' Max %','  All '/
      DATA(clab(2,i),i=1,6)/'Span 1','Span 2','Span 3','Span 4',
     &                      ' Diff.',' Spans'/
c-----------------------------------------------------------------------
      IF(Iagr.eq.6)THEN
       narg=1
      ELSE
       narg=0
      END IF
      DO i=1,5
       xran(i)=0D0
       DO j=1,Nsea
        IF(i.le.4)Kountr(i,j)=0
        IF(j.le.2)xmnx(i,j)=100D0
        xavg(i,j)=0D0
       END DO
      END DO
      DO i=1,Ncol
       DO j=1,Sslen
        IF((.not.dpeq(X(j,i),DNOTST)).and.j.ge.Ic)THEN
         jsea=mod(j,Nsea)
         IF(jsea.eq.0)jsea=Nsea
         xavg(i,jsea)=xavg(i,jsea)+X(j,i)
         xavg(Ns1,jsea)=xavg(Ns1,jsea)+X(j,i)
         IF(xmnx(i,1).gt.X(j,i))xmnx(i,1)=X(j,i)
         IF(xmnx(i,2).lt.X(j,i))xmnx(i,2)=X(j,i)
         Kountr(i,jsea)=Kountr(i,jsea)+1
        END IF
       END DO
       CALL compb(xavg,xmnx,xran,i,xcm,Ncol)
      END DO
      CALL compb(xavg,xmnx,xran,Ns1,xcm,Ncol)
      DO i=1,Nsea
       xmx=xavg(1,i)
       xmn=xavg(1,i)
       DO j=2,Ncol
        IF(xmx.lt.xavg(j,i))xmx=xavg(j,i)
        IF(xmn.gt.xavg(j,i))xmn=xavg(j,i)
       END DO
       xmpd(i)=((xmx-xmn)/xmn)*100D0
      END DO
      IF(Prttab(LSSFMN+narg))THEN
       fmt1='(1x,a9,1x, (f8.2,1x,a3),f7.2,1x,f7.2,1x,a3,/)'
       fmt2='(/,2(7x, (6x,a6),5x,a6,2x,a6,/))'
       fmt1(11:11)=numf(Ncol-1)
       fmt2(9:9)=numf(Ncol-1)
       WRITE(Mt1,fmt2)((clab(i,j),j=1,Ncol),clab(i,5),clab(i,6),i=1,2)
       k1=0
       IF(Nsea.eq.4)k1=12
       DO k=1,Nsea
        WRITE(Mt1,fmt1)Cpobs(k+k1),(xavg(j,k),xcm(j,k),j=1,Ncol),
     &                 xmpd(k),xavg(Ns1,k),xcm(Ns1,k)
       END DO
       sflab=' '
       nsflab=1
       IF(Iagr.eq.6)THEN
        sflab(1:9)='indirect '
        nsflab=9
       END IF
       IF(Muladd.eq.1)THEN
        sflab((nsflab+1):(nsflab+26))='implied adjustment factors'
        nsflab=25+nsflab
       ELSE
        sflab((nsflab+1):(nsflab+16))='seasonal factors'
        nsflab=15+nsflab
       END IF
       WRITE(Mt1,1010)sflab(1:nsflab)
 1010  FORMAT(/,'          Summary statistics for mean ',a)
       WRITE(Mt1,1020)
 1020  FORMAT(/,21x,'Min',12X,'Max',11X,'Range',/)
       DO k=1,Ncol
        WRITE(Mt1,1030)k,(xmnx(k,l),l=1,2),xran(k)
 1030   FORMAT(4x,'Span ',i1,3(5x,f10.2),/)
       END DO
       WRITE(Mt1,1040)(xmnx(Ns1,l),l=1,2),xran(Ns1)
 1040  FORMAT(2x,'All spans',4x,f10.2,2(5x,f10.2),/)
      END IF
      Lrange=T
      IF(xran(Ns1).lt.10)THEN
       Lrange=F
       WRITE(Mt1,1080)
 1080  FORMAT(/,5X,'WARNING: Range of seasonal factors is too low ',
     &        'for summary sliding spans measures to be reliable.',
     &        /,14x,'Summary sliding spans statistics not printed out')
       IF(narg.eq.0.and.((.not.Prttab(LSSTDS)).AND.Itd.eq.1))
     &    Prttab(LSSTDS)=T
       IF(.not.Prttab(LSSSFS+narg))Prttab(LSSSFS+narg)=T
       IF(.not.Prttab(LSSSAS+narg))Prttab(LSSSAS+narg)=T
       IF(.not.Prttab(LSSCHS+narg))Prttab(LSSCHS+narg)=T
       IF(.not.Prttab(LSSYCS+narg))Prttab(LSSYCS+narg)=T
c       IF(Prttab(LSSPCT+narg))Prttab(LSSPCT+narg)=F
c       IF(Prttab(LSSYPC+targ))Prttab(LSSYPC+targ)=F
c       IF(Prttab(LSSSUM+narg))Prttab(LSSSUM+narg)=F
c       IF(Savtab(LSSPCT+narg))Savtab(LSSPCT+narg)=F
      END IF
c-----------------------------------------------------------------------
      IF(Savtab(LSSFMN+narg))THEN
       IF(Iagr.eq.6)THEN
        lagr=6
        alab='issran'
       ELSE
        lagr=5
        alab='ssran'
       END IF
       fmt3='(a,a,i2.2,a,1x,a3,3x, (f10.2,2x),f10.2,2x,f10.2)'
       fmt3(22:22)=numf(Ncol-1)
       k1=0
       IF(Nsea.eq.4)k1=16
       DO k=1,Nsea
        WRITE(Nform,fmt3)alab(1:lagr),'.p',k,':',Cpobs(k+k1)(1:3),
     &                   (xavg(j,k),j=1,Ncol),xmpd(k),xavg(Ns1,k)
       END DO
       DO k=1,Ncol
        WRITE(Nform,1090)alab(1:lagr),k,(xmnx(k,l),l=1,2),xran(k)
 1090   FORMAT(a,'.s',i1,':',3(2x,f10.2))
       END DO
       WRITE(Nform,1100)alab(1:lagr),(xmnx(Ns1,l),l=1,2),xran(Ns1)
 1100  FORMAT(a,'.all:',3(2x,f10.2))
      END IF
      RETURN
      END
