C     Last change:  BCM  16 Feb 1999    3:39 pm
**==fgen.f    processed by SPAG 4.03F  at 11:18 on 14 Sep 1994
      SUBROUTINE fgen(Nw,Kfmt,Lf2,Lf3,Ldirect)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE GENERATES THE F2 AND F3 TABLES.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'title.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'mq3.cmn'
c-----------------------------------------------------------------------
      LOGICAL Lf2,Lf3,Ldirect
      CHARACTER mqf2*(7)
      INTEGER Kfmt,khcfm,Nw
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      IF(Lf2)THEN
       IF(Ny.eq.4)THEN
        mqf2=Moqu
       ELSE
        mqf2='  month'
       END IF
       khcfm=1
       IF(Kfmt.gt.0)khcfm=2
       IF(Lpage)THEN
        WRITE(Nw,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
        Kpage=Kpage+1
       END IF
       IF (Ldirect) THEN
         WRITE(Nw,1010)
       ELSE
         WRITE(Nw,1000)
       END IF
 1000  FORMAT(//,' F 2. Summary Measures for Indirect Adjustment')
 1010  FORMAT(//,' F 2. Summary Measures')
       IF(Lwdprt)THEN
        CALL prtf2w(Nw,mqf2,khcfm)
       ELSE
        CALL prtf2(Nw,mqf2,khcfm)
       END IF
      END IF
      IF(Lf3)THEN
       IF(Lpage)THEN
        WRITE(Nw,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
        Kpage=Kpage+1
       END IF
       IF (Ldirect) THEN
         WRITE(Nw,1020)
       ELSE
         WRITE(Nw,1030)
       END IF
 1030  FORMAT(//,' F 3. Monitoring and Quality Assessment Statistics for
     & Indirect Adjustment')
 1020  FORMAT(//,' F 3. Monitoring and Quality Assessment Statistics')
       CALL f3gen(Nw,Ny,Kfulsm,Lwdprt,Lcmpaq)
      END IF
      RETURN
      END
