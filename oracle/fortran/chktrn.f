      SUBROUTINE chktrn(Stc,Kpart,Ktabl,Trnchr,Tstfct,Oktrn)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      DOUBLE PRECISION ZERO,TWO
      LOGICAL F
      PARAMETER(F=.false.,TWO=2D0,ZERO=0D0)
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'notset.prm'
      INCLUDE 'x11ptr.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
c-----------------------------------------------------------------------
      CHARACTER cpart*1,Trnchr*1
      DOUBLE PRECISION Stc
      LOGICAL Oktrn,Tstfct,prtmsg
      INTEGER after,before,i,i2,i3,Kpart,Ktabl,last,kp
      DIMENSION Stc(*),cpart(4),Trnchr(*)
c-----------------------------------------------------------------------
      LOGICAL ispos
      EXTERNAL ispos
c-----------------------------------------------------------------------
      DATA cpart /'A','B','C','D'/
c-----------------------------------------------------------------------
      last=Posfob
      IF(Tstfct.and.Nfcst.gt.0)last=Posffc
      prtmsg=.not.ispos(Stc,Pos1bk,Posffc)
      IF(.not.prtmsg)THEN
       IF(Tstfct)Tstfct=F
       RETURN
      END IF
      Oktrn=ispos(Stc,Pos1ob,last)
c-----------------------------------------------------------------------
      DO i=Pos1bk,Posffc
       Trnchr(i)=' '
       IF(Stc(i).le.0)THEN
        Trnchr(i)='*'
c-----------------------------------------------------------------------
c     IF negative trend value found, print out warning message.
c-----------------------------------------------------------------------
        IF((.not.Lhiddn).and.prtmsg)THEN
         kp=Kpart
         if(kp.eq.6)kp=4
         IF(.not.Lquiet)WRITE(STDERR,1010)cpart(kp),Ktabl
         CALL errhdr
         WRITE(Mt2,1010)cpart(kp),Ktabl
 1010    FORMAT(/,' WARNING: At least one negative value was found in',
     &          ' one of the trend',
     &          /,'          cycle estimates (',a,i3,').  Negative ',
     &          'value(s) will be replaced',
     &          /,'          either by the mean of its two closest ',
     &          'neighbors that are greater',
     &          /,'          than zero, or by the nearest value that ',
     &            ' is greater than zero',
     &          /,'          (if the value is on either end of the ',
     &            'series).')
         prtmsg=F
        END IF
c-----------------------------------------------------------------------
c     Replace negative trend value with either the mean of the two 
c     nearest positive replacements before and after the value, or
c     the nearest value if it is on the ends of the series.
c-----------------------------------------------------------------------
        i2=1
        before=0
        after=0
        DO WHILE (before.eq.0.or.after.eq.0)
         IF(before.eq.0)THEN
          i3=i-i2
          IF(i3.lt.Pos1ob)THEN
           before=NOTSET
          ELSE IF(Stc(i3).gt.ZERO)THEN
           before=i3
          END IF
         END IF
         IF(after.eq.0)THEN
          i3=i+i2
          IF(i3.gt.Posfob)THEN
           after=NOTSET
          ELSE IF(Stc(i3).gt.ZERO)THEN
           after=i3
          END IF
         END IF
         i2=i2+1
        END DO
        IF(before.eq.NOTSET)THEN
         Stc(i)=Stc(after)
        ELSE IF(after.eq.NOTSET)THEN
         Stc(i)=Stc(before)
        ELSE
         Stc(i)=(Stc(after)+Stc(before))/TWO
        END IF
       END IF
      END DO
      IF(Lhiddn)RETURN
c-----------------------------------------------------------------------
      IF(Oktrn)THEN
       IF(.not.prtmsg)THEN
        WRITE(STDERR,1020)
        CALL errhdr
        WRITE(Mt2,1020)
 1020   FORMAT(/,'          These are often caused by poor forecasts ',
     &           'and/or backcasts.',
     &         /,'          Users should check the fit of any existing',
     &          ' regARIMA model,',
     &         /,'          using the diagnostics in the check spec.')
       END IF
      ELSE
       IF(Nfcst.gt.0.or.Nbcst.gt.0)THEN
        WRITE(STDERR,1030)
        CALL errhdr
        WRITE(Mt2,1030)
 1030   FORMAT(/,'          These are often caused by substantial ',
     &           'outliers in the original',
     &         /,'          series, or poor forecasts and/or ',
     &           'backcasts.  Users should',
     &         /,'          fit a regARIMA model to the series using ',
     &           'outlier regression',
     &         /,'          variables to correct for such effects, and',
     &           ' check the fit of',
     &         /,'          any existing regARIMA model, using the ',
     &           'diagnostics in the',
     &         /,'          check spec.')
       ELSE
        WRITE(STDERR,1040)
        CALL errhdr
        WRITE(Mt2,1040)
 1040   FORMAT(/,'          These are often caused by substantial ',
     &           'outliers in the original',
     &         /,'          series.  Users should fit a regARIMA ',
     &           'model to the series',
     &         /,'          using outlier regression variables to ',
     &           'correct for such effects.')
       END IF
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
