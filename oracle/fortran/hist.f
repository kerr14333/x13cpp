C     Last change: Mar. 21, - add Tabular Histogram of the Standardized
C     and Mean-Centered Residuals
C     previous change:  BCM  25 Nov 97   11:58 am
      SUBROUTINE hist(Y,Begspn,Sp,Nobs,D,Muladd)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     hist.f, Release 1, Subroutine Version 1.5, Modified 03 Nov 1994.
c-----------------------------------------------------------------------
c     Calculates the histogram of the ny long data vector, y.
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
c median    d  Local median of the y's
c nbin    i  Local number of bins or columns in the histogram
c notlr   i  Local number of outliers (in otlr)
c nobs    i  Input number of observations
c nrow    i  Local number of rows in the histogram
c otlr    i  Local notlr long list of the values of residuals
c             greater than 3.25
c otlrt0  i  Local notlr long list of residuals greater than 3.25
c             standard deviations from the median
c scale   d  Local scale factor to make the number of rows be 40
c stddev  d  Local standard deviation of the y's
c sum     d  Local scalar sum of y
c sumsq   d  Local scalar sum of squares of y
c tmp     d  Local temporary scalar
c width   d  Local width between cut points
c y       d  Input nobs long vector of observations
c ymax    d  Local scalar maximum of the y's
c ymin    d  Local scalar minimum of the y's
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      INTEGER PA,POTLEN
      DOUBLE PRECISION ONE
      PARAMETER(PA=PLEN+2*PORDER,ONE=1D0,POTLEN=PLEN/4)
c     ------------------------------------------------------------------
      CHARACTER str*(10)
      INTEGER Begspn,bin,D,i,ibin,idate,irow,midpt,nbin,nchr,Nobs,notlr,
     &        nrow,otlrt0,Sp,Muladd
      DOUBLE PRECISION cutpnt,lowbnd,median,otlr,srtdy,stddev,tmp,width,
     &                 Y,ymax,ymin,ceilng,scale
      DIMENSION Begspn(2),bin(15),cutpnt(15),idate(2),otlr(POTLEN),
     &          otlrt0(POTLEN),srtdy(PA),Y(Nobs)
      EXTERNAL ceilng
c-----------------------------------------------------------------------
c     Find the minimum, maximum, median, and standard deviation of the
c observations.
c-----------------------------------------------------------------------
      CALL copy(Y,Nobs,1,srtdy)
      CALL shlsrt(Nobs,srtdy)
c     ------------------------------------------------------------------
      ymin=srtdy(1)
      ymax=srtdy(Nobs)
c     ------------------------------------------------------------------
      midpt=Nobs/2
      IF(mod(Nobs,2).eq.0)THEN
       median=(srtdy(midpt)+srtdy(midpt+1))/2D0
      ELSE
       median=srtdy(midpt+1)
      END IF
c     ------------------------------------------------------------------
      CALL medabs(Y,Nobs,stddev)
      IF(Lfatal)RETURN
      stddev=1.49D0*stddev
c-----------------------------------------------------------------------
c     Find the range and lower bound of the histogram.
c-----------------------------------------------------------------------
      width=.5D0
      lowbnd=-3.25D0
c-----------------------------------------------------------------------
c     Calculate the cut points.
c-----------------------------------------------------------------------
      tmp=lowbnd
      nbin=15
c     ------------------------------------------------------------------
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
       tmp=(Y(i)-median)/stddev
c     ------------------------------------------------------------------
       IF(tmp.lt.lowbnd)THEN
        notlr=notlr+1
        IF(notlr.le.POTLEN)THEN
         otlrt0(notlr)=i+D
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
        IF(notlr.le.POTLEN)THEN
         otlrt0(notlr)=i+D
         otlr(notlr)=tmp
        END IF
        bin(nbin)=bin(nbin)+1
        nrow=max(nrow,bin(nbin))
       END IF
   10  CONTINUE
      END DO
c     ------------------------------------------------------------------
      scale=ceilng(dble(nrow)/69D0)
      IF(scale.gt.ONE)THEN
       DO ibin=1,nbin
        bin(ibin)=int(bin(ibin)/scale)
       END DO
      END IF
c-----------------------------------------------------------------------
c     Print the histogram sideways with negative values on top.
c-----------------------------------------------------------------------
      WRITE(Mt1,1010)
 1010 FORMAT('  Standard',/,'  Deviations     Frequency')
c     ------------------------------------------------------------------
      IF(bin(1).gt.0)THEN
       WRITE(Mt1,1020)('#',i=1,bin(1))
 1020  FORMAT(/,'  Outlier [',69A1)
       WRITE(Mt1,'(1x)')
      END IF
c     ------------------------------------------------------------------
      DO irow=2,nbin-2,2
       WRITE(Mt1,1030)(irow-8)/2,('#',i=1,bin(irow))
 1030  FORMAT(i9,' +',69A1)
       WRITE(Mt1,1040)('#',i=1,bin(irow+1))
 1040  FORMAT(9x,' |',69A1)
      END DO
      WRITE(Mt1,1030)(nbin-9)/2,('#',i=1,bin(nbin-1))
c     ------------------------------------------------------------------
      IF(bin(nbin).gt.0)WRITE(Mt1,1020)('#',i=1,bin(nbin))
c     ------------------------------------------------------------------
      WRITE(Mt1,1050)int(scale)
 1050 FORMAT(/,'  One ''#''=',i2,' observation[s]')
c     ------------------------------------------------------------------
      WRITE(Mt1,1000)
 1000 FORMAT(/,' Tabular Histogram of the Standardized and '
     $'Mean-Centered Residuals',/)
      WRITE(Mt1,1010)
      IF(bin(1).gt.0)THEN
        WRITE(Mt1,1010)
 1001   FORMAT(/,'Outlier','     ',t21,i4)
      END IF
      DO irow=2,nbin-1
        IF(Muladd.eq.1)THEN
          WRITE(Mt1,1002)(cutpnt(irow)+cutpnt(irow-1))/2,bin(irow)
 1002     FORMAT(/,f7.1,t21,i4)
        ELSE
          WRITE(Mt1,1003)(cutpnt(irow)+cutpnt(irow-1))/2,bin(irow)
 1003     FORMAT(/,f6.2,t21,i4)
        END IF
      END DO
c     ------------------------------------------------------------------
      IF(notlr.gt.0)THEN
       WRITE(Mt1,1060)
 1060  FORMAT(/,'  Residuals with |t|>3.25')
       IF(notlr.gt.POTLEN)THEN
        WRITE(Mt1,1090)POTLEN,notlr
 1090   FORMAT(/,'  Only the first ',i3,' of the ',i3,
     &           ' extreme residuals are shown.',/)
       END IF
       WRITE(Mt1,1061)
 1061  FORMAT(/,'  Obs       t-value',/,'  -----------------')
       DO i=1,notlr
        IF(i.le.POTLEN)THEN
         CALL addate(Begspn,Sp,otlrt0(i)-(D+1),idate)
         CALL wrtdat(idate,Sp,str,nchr)
         IF(Lfatal)RETURN
         WRITE(Mt1,1070)str(1:nchr),otlr(i)
 1070    FORMAT('  ',a,t12,f8.2)
        END IF
       END DO
      END IF
c     ------------------------------------------------------------------
      WRITE(Mt1,1080)ymin,ymax,median,stddev
 1080 FORMAT(/,' Summary Statistics for the Unstandardized Residuals',/,
     &       '  Minimum',t21,f15.3,/,'  Maximum',t21,f15.3,/,'  Median',
     &       t21,f15.3,/,'  Robust Std Dev',t21,f15.3)
c     ------------------------------------------------------------------
      RETURN
      END
