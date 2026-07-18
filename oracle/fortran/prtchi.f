      SUBROUTINE prtchi(Fh,Lprhdr,Tbwdth,Baselt,Grpstr,Nchr,Info,Df,
     &                  Chi2vl,Pv,Hdrstr)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'title.cmn'
c-----------------------------------------------------------------------
      CHARACTER Grpstr*(PGRPCR),Hdrstr*(*)
      LOGICAL Lprhdr
      INTEGER Fh,Tbwdth,Baselt,Nchr,Info,Df,i
      DOUBLE PRECISION Chi2vl,Pv
c-----------------------------------------------------------------------
      IF(Lprhdr)THEN
       IF(.not.Lcmpaq)WRITE(Fh,'()')
       WRITE(Fh,1010)Hdrstr
       WRITE(Fh,1020)('-',i=1,tbwdth)
       WRITE(Fh,1030)
       WRITE(Fh,1020)('-',i=1,tbwdth)
      END IF
c-----------------------------------------------------------------------
      IF(Info.eq.0)THEN
       IF(Baselt.eq.NOTSET)THEN
        WRITE(Fh,1080)Grpstr(1:Nchr)
       ELSE
        IF(Nchr.gt.34)THEN
         WRITE(Fh,1050)Grpstr(1:Nchr),Df,Chi2vl,Pv
        ELSE
         WRITE(Fh,1060)Grpstr(1:Nchr),Df,Chi2vl,Pv
        END IF
       END IF
c-----------------------------------------------------------------------
      ELSE
       WRITE(Fh,1070)Grpstr(1:Nchr)
      END IF
c-----------------------------------------------------------------------
      RETURN
c-----------------------------------------------------------------------
 1010 FORMAT(/,' Chi-squared Tests for Groups of ',a)
 1020 FORMAT(' ',120(a))
 1030 FORMAT(' Regression Effect',t37,'df',t45,'Chi-Square',t61,
     &        'P-Value')
 1050 FORMAT(' ',a,/,t35,i4,f16.2,f13.2)
 1060 FORMAT(' ',a,t35,i4,f16.2,f13.2)
 1070 FORMAT(' ',a,t52,'Not tested')
 1080 FORMAT(' ',a,t41,'All coefficients fixed')
c-----------------------------------------------------------------------
	  END
