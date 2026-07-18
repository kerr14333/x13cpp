C     Last change:  BCM  22 Jul 1998    9:40 am
**==qcontr.f    processed by SPAG 4.03F  at 09:52 on  1 Mar 1994
      SUBROUTINE qcontr(Mal,Mq)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE PRINTS THE QUALITY CONTROL STATISTICS IN A
C --- SUMMARIZED FORM AT THE END OF THE PRINTOUT.
c-----------------------------------------------------------------------
      INCLUDE 'agr.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
c-----------------------------------------------------------------------
      CHARACTER aggs*(8),atype*(8)
      INTEGER i,k,Mal,Mq
      DIMENSION aggs(3),atype(10)
c-----------------------------------------------------------------------
      DATA atype/'M-AUTO  ','M-NONE  ','M-MLT   ','M-ADD   ','M-LOG   ',
     &           'Q-AUTO  ','Q-NONE  ','Q-MLT   ','Q-ADD   ','Q-LOG   '/
      DATA aggs/'        ','DIRECT  ','INDIRECT'/
c-----------------------------------------------------------------------
      k=Iagr-1
      IF(k.lt.1)k=1
      i=Mal+3
      IF(Mq.eq.4)i=i+5
c-----------------------------------------------------------------------
      WRITE(Ng,1010)atype(i),Serno(1:6),Title(1:40),aggs(k)
 1010 FORMAT(/,2X,A7,2X,A6,' -------- -------- ',A40,2x,A8)
      IF(Ntitle.gt.40)WRITE(Ng,1020)Title(41:Ntitle)
 1020 FORMAT(36X,A)
c-----------------------------------------------------------------------
      RETURN
      END
