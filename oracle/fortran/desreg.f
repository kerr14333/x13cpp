C     Last change:  BCM  13 May 1998    9:04 am
      SUBROUTINE desreg(Ttlstr,Ngrp,Grpttl,Grpptr,Ngrptl)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Constructs a description of the regression model
c At some point need to show that the matrix might be length of month
c adjusted
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'error.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'units.cmn'
c     ------------------------------------------------------------------
      CHARACTER addon*3,str*(PGRPCR),tmpttl*80,Grpttl*(PGRPCR*PGRP),
     &          Ttlstr*(*)
      INTEGER igrp,naddcr,nchr,nttlcr,Grpptr,Ngrptl,Ngrp
      DIMENSION Grpptr(0:PGRP)
c-----------------------------------------------------------------------
c     Print the regression part of the model
c-----------------------------------------------------------------------
      nttlcr=1
      CALL setchr(' ',80,tmpttl)
      IF(Lcmpaq)THEN
       nttlcr=1+LEN(Ttlstr)
       tmpttl(2:nttlcr)=Ttlstr(1:len(Ttlstr))
       IF(nttlcr.lt.21)nttlcr=21
      ELSE
       WRITE(Mt1,1010)Ttlstr
 1010  FORMAT(/,' ',a)
      END IF
      addon='   '
      naddcr=1
c     ------------------------------------------------------------------
      DO igrp=1,Ngrp
       CALL getstr(Grpttl,Grpptr,Ngrptl,igrp,str,nchr)
       IF(Lfatal)RETURN
c     ------------------------------------------------------------------
       IF(nttlcr+nchr+naddcr.ge.78)THEN
        WRITE(Mt1,1020)tmpttl(1:nttlcr)//addon(1:naddcr)
 1020   FORMAT(a)
        nttlcr=2+nchr
        tmpttl(1:nttlcr)='  '//str(1:nchr)
c     ------------------------------------------------------------------
       ELSE
        tmpttl(nttlcr+1:nttlcr+nchr+naddcr)=addon(1:naddcr)
     &    //str(1:nchr)
        nttlcr=nttlcr+nchr+naddcr
        addon=' + '
        naddcr=3
       END IF
      END DO
c     ------------------------------------------------------------------
      WRITE(Mt1,1020)tmpttl(1:nttlcr)
c     ------------------------------------------------------------------
      RETURN
      END

