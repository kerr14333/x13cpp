C     Last change:  BCM  16 Feb 1999   11:16 am
**==histx.f    processed by SPAG 4.03F  at 14:08 on 24 Aug 1994
      SUBROUTINE histx(Y,Nobs,Muladd,Ny,Lyr,Begsrs,Itbl,Ldiff,Lprt,Lsav,
     &                 Label)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Calculates the histogram of the ny long revisions vector, y.
c-----------------------------------------------------------------------
c bin     i  Local vector of counts of observations between cut points
c             i.e. bins
c cutpnt  d  Local vector of cut points where observations counted in
c             bin(i) are cutpnt(i-1) < y <= cutpnt(i)
c d       i  Input diffence between the number of observations in the
c             series and number of effective observations in the series,
c             nobs-nefobs
c i       i  Local do loop index
c ibin    i  Local index for the current bin, or column of the
c             histogram
c irow    i  Local index for the current row of the output
c lowbnd  d  Local scalar for the low bound of the observations
c nbin    i  Local number of bins or columns in the histogram
c notlr   i  Local number of outliers (in otlr)
c nobs    i  Input number of observations
c nrow    i  Local number of rows in the histogram
c otlr    i  Local notlr long list of the values of residuals
c             greater than 3.25
c otlrt0  i  Local notlr long list of residuals greater than 3.25
c             standard deviations from the median
c xscale  d  Local scale factor to make the number of rows be 40
c stddev  d  Local standard deviation of the y's
c sum     d  Local scalar sum of y
c sumsq   d  Local scalar sum of squares of y
c tmp     d  Local temporary scalar
c width   d  Local width between cut points
c y       d  Input nobs long vector of revisions
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'tfmts.cmn'
c     ------------------------------------------------------------------
      DOUBLE PRECISION ONE
      PARAMETER(ONE=1D0)
      INTEGER MO,YR
      PARAMETER(MO=2,YR=1)
c-----------------------------------------------------------------------
      LOGICAL Lprt,Lsav,Ldiff
      CHARACTER Label*(*),dash*50,ex*(2)
      INTEGER bin,i,ibin,irow,nbin,Nobs,notlr,nrow,otlrt0,Muladd,
     &        otlobs,Ny,Lyr,Begsrs,iobs,Itbl,i2,ic
      DOUBLE PRECISION cutpnt,lowbnd,otlr,stddev,tmp,width,Y,ts,tsxtra,
     &                 temp,ceilng,xscale
      DIMENSION bin(15),cutpnt(15),otlr(50),otlrt0(50),ts(5),Y(*),
     &          otlobs(2),temp(PLEN),ex(2*NEST)
      EXTERNAL ceilng
c-----------------------------------------------------------------------
      DATA dash/'  ------------------------------------------------'/
      DATA ex/'a ','ai','b ','bi','c ','ci','d ','di','e ','ei'/
c-----------------------------------------------------------------------
      CALL copy(Y,Nobs,1,temp)
      ic=(Itbl-1)/2+1
      CALL hinge(temp,Nobs,ts,tsxtra,ic)
c-----------------------------------------------------------------------
c     Find standard deviation of the observations.
c-----------------------------------------------------------------------
      CALL medabs(Y,Nobs,stddev)
      IF(Lfatal)RETURN
      stddev=stddev/0.6745D0
c      IF(stddev.eq.0)RETURN
c     ------------------------------------------------------------------
c     Print Summary statistics
c     ------------------------------------------------------------------
      IF(Lprt)THEN
       WRITE(Mt1,1110)Label
 1110  FORMAT(//,' Summary Statistics for the ',a,/)
       IF(Ldiff)THEN
        IF(Tblwid.gt.7)THEN
         WRITE(Mt1,1121)(ts(i),i=1,5),stddev
        ELSE
         WRITE(Mt1,1120)(ts(i),i=1,5),stddev
        END IF
 1121   FORMAT(5x,'Minimum',t27,': ',5x,g17.10,/,5x,'25th Percentile',
     &         t27,': ',5x,g17.10,/,5x,'Median',t27,': ',5x,g17.10,/,5x,
     &         '75th Percentile',t27,': ',5x,g17.10,/,5x,'Maximum',t27,
     &         ': ',5x,g17.10,//,5x,'Standard Deviation',t27,': ',5x,
     &         g17.10,//)
 1120   FORMAT(5x,'Minimum',t27,': ',5x,f10.1,/,5x,'25th Percentile',
     &         t27,': ',5x,f10.1,/,5x,'Median',t27,': ',5x,f10.1,/,5x,
     &         '75th Percentile',t27,': ',5x,f10.1,/,5x,'Maximum',t27,
     &         ': ',5x,f10.1,//,5x,'Standard Deviation',t27,': ',5x,
     &         f10.1,//)
       ELSE
c     ------------------------------------------------------------------
c     Print hinge values for sliding spans analysis.
c     ------------------------------------------------------------------
        WRITE(Mt1,1130)(ts(i),i=1,3)
 1130   FORMAT(5x,'Minimum',t27,': ',f10.2,/,5x,'25th Percentile',t27,
     &         ': ',f10.2,/,5x,'Median',t27,': ',f10.2)
        IF(ic.le.3)THEN
         WRITE(Mt1,1131)ts(4),85,tsxtra,ts(5),stddev
 1131    FORMAT(5x,'75th Percentile',t27,': ',f10.2,/,3x,'->',i2,
     &          'th Percentile',t27,': ',f10.2,'<-',/,5x,'Maximum',
     &          t27,': ',f10.2//,5x,'Standard Deviation',t27,': ',5x,
     &          f10.2,//)
        ELSE IF(ic.eq.4)THEN
         WRITE(Mt1,1132)tsxtra,(ts(i),i=4,5),stddev
 1132    FORMAT(3x,'->60th Percentile',t27,': ',f10.2,'<-',/,5x,
     &          '75th Percentile',t27,': ',f10.2,/,5x,'Maximum',
     &          t27,': ',f10.2//,5x,'Standard Deviation',t27,': ',5x,
     &          f10.2,//)
        ELSE
         WRITE(Mt1,1131)ts(4),90,tsxtra,ts(5),stddev
        END IF
       END IF
      END IF
c-----------------------------------------------------------------------
c     Find the range and lower bound of the histogram.
c-----------------------------------------------------------------------
      width=.25D0*stddev
      lowbnd=0
      IF(Ldiff)THEN
       nbin=15
      ELSE
       nbin=12
      END IF
c-----------------------------------------------------------------------
c     Calculate the cut points.
c-----------------------------------------------------------------------
      tmp=lowbnd
      DO i=1,nbin
       cutpnt(i)=tmp
       tmp=tmp+width
      END DO
c-----------------------------------------------------------------------
c     Sort the observations into bins.
c-----------------------------------------------------------------------
      nrow=0
      notlr=0
      CALL setint(0,nbin,bin)
c     ------------------------------------------------------------------
      DO i=1,Nobs
       tmp=Y(i)
c     ------------------------------------------------------------------
       IF(tmp.lt.lowbnd)THEN
        notlr=notlr+1
        IF (notlr.le.50) THEN
         otlrt0(notlr)=i
         otlr(notlr)=tmp
        END IF
        bin(1)=bin(1)+1
        nrow=max(nrow,bin(1))
c     ------------------------------------------------------------------
       ELSE
        DO ibin=2,nbin-1
         IF(tmp.lt.cutpnt(ibin))THEN
          bin(ibin)=bin(ibin)+1
          nrow=max(nrow,bin(ibin))
          GO TO 10
         END IF
        END DO
c     ------------------------------------------------------------------
        notlr=notlr+1
        IF(notlr.le.50)THEN
         otlrt0(notlr)=i
         otlr(notlr)=tmp
        END IF
        bin(nbin)=bin(nbin)+1
        nrow=max(nrow,bin(nbin))
       END IF
   10  CONTINUE
      END DO
c     ------------------------------------------------------------------
      xscale=ceilng(dble(nrow)/69D0)
      IF(xscale.gt.ONE)THEN
       DO ibin=1,nbin
        bin(ibin)=int(bin(ibin)/xscale)
       END DO
      END IF
c-----------------------------------------------------------------------
c     Print the histogram sideways with negative values on top.
c-----------------------------------------------------------------------
      IF(Lprt)THEN
       WRITE(Mt1,1010)Label
 1010  FORMAT(//,' Histogram of the ',a,/)
       IF(Ldiff)THEN
        IF(Tblwid.gt.5)THEN
         WRITE(Mt1,1021)
        ELSE
         WRITE(Mt1,1020)
       END IF
 1021   FORMAT('  Absolute Differences      Frequency')
 1020   FORMAT(' Absolute',/,' Differences      Frequency')
       ELSE
        WRITE(Mt1,1030)
 1030   FORMAT(' Percent',/,' Differences      Frequency')
       END IF
c     ------------------------------------------------------------------
       IF(bin(1).gt.0)THEN
        IF(Ldiff.and.Tblwid.gt.5)THEN
         WRITE(Mt1,1040)'             Outlier [',('#',i=1,bin(1))
        ELSE
         WRITE(Mt1,1040)'   Outlier [',('#',i=1,bin(1))
        END IF
 1040   FORMAT(/,A,69A1)
        WRITE(Mt1,'(1x)')
       END IF
c     ------------------------------------------------------------------
       DO irow=2,nbin-2,2
        IF(Muladd.eq.1)THEN
         IF(Ldiff.and.Tblwid.gt.5)THEN
          WRITE(Mt1,1051)(cutpnt(irow)+cutpnt(irow-1))/2,
     &                   ('#',i=1,bin(irow))
          WRITE(Mt1,1070)'           |',('#',i=1,bin(irow+1))
         ELSE
          WRITE(Mt1,1050)(cutpnt(irow)+cutpnt(irow-1))/2,
     &                   ('#',i=1,bin(irow))
          WRITE(Mt1,1070)' |',('#',i=1,bin(irow+1))
         END IF
 1051    FORMAT(3x,G17.10,' +',69A1)
 1050    FORMAT(3x,f7.1,' +',69A1)
        ELSE
         WRITE(Mt1,1060)(cutpnt(irow)+cutpnt(irow-1))/2,
     &                  ('#',i=1,bin(irow))
         WRITE(Mt1,1070)' |',('#',i=1,bin(irow+1))
 1060    FORMAT(4x,f6.2,' +',69A1)
        END IF
 1070   FORMAT(10x,a,69A1)
       END DO
       IF(Muladd.eq.1)THEN
        IF(Ldiff.and.Tblwid.gt.5)THEN
         WRITE(Mt1,1051)(cutpnt(nbin-1)+cutpnt(nbin-2))/2,
     &                  ('#',i=1,bin(nbin-1))
        ELSE
         WRITE(Mt1,1050)(cutpnt(nbin-1)+cutpnt(nbin-2))/2,
     &                  ('#',i=1,bin(nbin-1))
        END IF
       ELSE
        WRITE(Mt1,1060)(cutpnt(nbin-1)+cutpnt(nbin-2))/2,
     &                 ('#',i=1,bin(nbin-1))
       END IF
c     ------------------------------------------------------------------
       IF(bin(nbin).gt.0)WRITE(Mt1,1040)('#',i=1,bin(nbin))
c     ------------------------------------------------------------------
       WRITE(Mt1,1080)int(xscale)
 1080  FORMAT(/,'  One ''#''=',i2,' observation[s]')
c     ------------------------------------------------------------------
       IF(notlr.gt.0)THEN
        WRITE(Mt1,1090)Label,Label,dash(1:len(Label)+9)
 1090   FORMAT(/,2x,a,' considered to be outliers',/,'  Time   ',a,/,a)
        DO i=1,MIN(notlr,50)
         iobs=otlrt0(i)+Begsrs-1
         otlobs(MO)=mod(iobs,Ny)
         IF(otlobs(MO).eq.0)otlobs(MO)=Ny
         otlobs(YR)=Lyr+(iobs-1)/Ny
         IF(Tblwid.gt.5)THEN
          WRITE(Mt1,1101)otlobs(MO),otlobs(YR),otlr(i)
         ELSE
          WRITE(Mt1,1100)otlobs(MO),otlobs(YR),otlr(i)
         END IF
 1101    FORMAT(1x,i2,':',i4,G17.10)
 1100    FORMAT(1x,i2,':',i4,f8.2)
        END DO
        IF(notlr.gt.50)WRITE(Mt1,1160)notlr
 1160   FORMAT(//,
     &     '  Number of observations considered to be outliers = ',i3,/,
     &     '  (only the first 50 were listed above)')
       END IF
      END IF
c     ------------------------------------------------------------------
c     Save hinge values
c     ------------------------------------------------------------------
      IF(Lsav.and.Nform.gt.0)THEN
       i2=2-mod(Itbl,2)
       IF(Ldiff)THEN
        IF(ic.eq.4)THEN
         WRITE(Nform,1151)ex(Itbl)(1:i2),(ts(i),i=1,3),tsxtra,(ts(i),
     &                    i=4,5),stddev
        ELSE
         WRITE(Nform,1151)ex(Itbl)(1:i2),(ts(i),i=1,4),tsxtra,ts(5),
     &                    stddev
        END IF
       ELSE
        IF(ic.eq.4)THEN
         WRITE(Nform,1150)ex(Itbl)(1:i2),(ts(i),i=1,3),tsxtra,(ts(i),
     &                    i=4,5),stddev
        ELSE
         WRITE(Nform,1150)ex(Itbl)(1:i2),(ts(i),i=1,4),tsxtra,ts(5),
     &                    stddev
        END IF
       END IF
 1151  FORMAT('s3.',a,'.hinge:',8(2x,g17.10))
 1150  FORMAT('s3.',a,'.hinge:',8(2x,f8.3))
      END IF
c     ------------------------------------------------------------------
      RETURN
c     ------------------------------------------------------------------
      END
