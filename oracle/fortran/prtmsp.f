C     Last change:  BCM  24 Nov 97   11:16 am
      SUBROUTINE prtmsp(Begdat,Enddat,Sp,Lxreg)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Prints the model span for a given model estimation
c-----------------------------------------------------------------------
      INCLUDE 'title.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      LOGICAL Lxreg
      CHARACTER bdtstr*(10),edtstr*(10)
      INTEGER Begdat,Enddat,nchr1,nchr2,Sp
      DIMENSION Begdat(2),Enddat(2)
c     ------------------------------------------------------------------
c     create character versions of the beginning and ending dates
c     ------------------------------------------------------------------
      CALL wrtdat(Begdat,Sp,bdtstr,nchr1)
      IF(.not.Lfatal)CALL wrtdat(Enddat,Sp,edtstr,nchr2)
      IF(Lfatal)RETURN
c     ------------------------------------------------------------------
c     Write them out
c     ------------------------------------------------------------------
      IF(Lcmpaq)THEN
       IF(Lxreg)THEN
        WRITE(Mt1,1010)' Irregular Component Regression Span',
     &                 bdtstr(1:nchr1),edtstr(1:nchr2)
       ELSE
        WRITE(Mt1,1010)' regARIMA Model Span',bdtstr(1:nchr1),
     &                 edtstr(1:nchr2)
       END IF
      ELSE
       IF(Lxreg)THEN
        WRITE(Mt1,1020)' Irregular Component Regression Span',
     &                 bdtstr(1:nchr1),edtstr(1:nchr2)
       ELSE
        WRITE(Mt1,1020)' regARIMA Model Span',bdtstr(1:nchr1),
     &                 edtstr(1:nchr2)
       END IF
      END IF
c     ------------------------------------------------------------------
 1010 FORMAT(a,': ',a,' to ',a)
 1020 FORMAT(/,a,/,'  From  ',a,' to ',a)
c     ------------------------------------------------------------------
      RETURN
      END
