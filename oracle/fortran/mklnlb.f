      SUBROUTINE mklnlb(Lnstr,Nlnchr,Lnabb,Nlnabb,Lomtst,Aicrgm,Lnzero,
     &                  Sp)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     Generate string for label of lom/loq/lpyear effect within AIC test
c     for lom/loq/lpyear regressors (BCM March 2008)
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER Lnstr*(30),Lnabb*(6),datstr*(10)
      INTEGER Lomtst,Aicrgm,Nlnchr,Nlnabb,nchdat,Lnzero,Sp
      DIMENSION Aicrgm(2)
c-----------------------------------------------------------------------
c    Initialize Lnstr with blanks
c-----------------------------------------------------------------------
      CALL setchr(' ',30,Lnstr)
      CALL setchr(' ',6,Lnabb)
c-----------------------------------------------------------------------
c    Set base of Lnstr
c-----------------------------------------------------------------------
      IF(Lomtst.eq.1)THEN
       Nlnchr=3
       Lnstr(1:Nlnchr)='lom'
      ELSE IF(Lomtst.eq.2)THEN
       Nlnchr=3
       Lnstr(1:Nlnchr)='loq'
      ELSE IF(Lomtst.eq.3)THEN
       Nlnchr=6
       Lnstr(1:Nlnchr)='lpyear'
      END IF
c-----------------------------------------------------------------------
      Lnabb(1:Nlnchr)=Lnstr(1:Nlnchr)
      Nlnabb=Nlnchr
c-----------------------------------------------------------------------
c    Add change of regime date, if necessary
c-----------------------------------------------------------------------
      IF(Aicrgm(1).ne.NOTSET)THEN
       CALL wrtdat(Aicrgm,Sp,datstr,nchdat)
       IF(Lfatal)RETURN
       IF(Lnzero.eq.0)THEN
        Lnstr((Nlnchr+1):(Nlnchr+nchdat+2))='/'//datstr(1:nchdat)//'/'
        Nlnchr=Nlnchr+nchdat+2
       ELSE IF(Lnzero.eq.1)THEN
        Lnstr((Nlnchr+1):(Nlnchr+nchdat+3))='/'//datstr(1:nchdat)//'//'
        Nlnchr=Nlnchr+nchdat+3
       ELSE IF(Lnzero.eq.2)THEN
        Lnstr((Nlnchr+1):(Nlnchr+nchdat+4))='//'//datstr(1:nchdat)//'//'
        Nlnchr=Nlnchr+nchdat+4
       ELSE
        Lnstr((Nlnchr+1):(Nlnchr+nchdat+3))='//'//datstr(1:nchdat)//'/'
        Nlnchr=Nlnchr+nchdat+3
       END IF
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
      