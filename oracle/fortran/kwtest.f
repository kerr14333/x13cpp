C     Last change:  BCM  25 Nov 97   11:48 am
**==kwtest.f    processed by SPAG 4.03F  at 15:12 on  1 Aug 1994
      SUBROUTINE kwtest(X,Ib,Ie,Nyr,Lprt)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE APPLIES THE KRUSKAL-WALLIS TEST TO X. THE K-W TEST
C --- IS THE NONPARAMETRIC EQUIVALENT OF THE F-TEST.
C --- THE ARRAY X IS DESTROYED IN THE CALCULATION PROCEDURE.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'tests.cmn'
c-----------------------------------------------------------------------
      LOGICAL Lprt
      CHARACTER xb*50
      DOUBLE PRECISION ck,X,chisq,xval
      INTEGER i,Ib,Ie,j,k,kolr,kval,l,n,ndf,ns,Nyr,sp1
      DIMENSION X(Ie),ns(PSP),kolr(PSP),k(PLEN)
      EXTERNAL chisq
c-----------------------------------------------------------------------
C --- INITIALIZE.
c-----------------------------------------------------------------------
      DO i=1,Nyr
       kolr(i)=0
       ns(i)=0
      END DO
      DO i=Ib,Ie
       k(i)=i
      END DO
c-----------------------------------------------------------------------
C --- RANK THE ARRAY X.
c-----------------------------------------------------------------------
      DO i=Ib,Ie
       xval=X(i)
       kval=k(i)
       DO j=i,Ie
        IF(xval.gt.X(j))THEN
         X(i)=X(j)
         k(i)=k(j)
         X(j)=xval
         k(j)=kval
         xval=X(i)
         kval=k(i)
        END IF
       END DO
      END DO
c-----------------------------------------------------------------------
C --- CALCULATE THE COLUMN SUM OF RANKS.
c-----------------------------------------------------------------------
      DO i=Ib,Ie
       l=k(i)-(k(i)-1)/Nyr*Nyr
       ns(l)=ns(l)+1
       kolr(l)=i-Ib+1+kolr(l)
      END DO
c-----------------------------------------------------------------------
C --- CALCULATE THE K-W STATISTIC.
c-----------------------------------------------------------------------
      ck=0D0
      DO i=1,Nyr
       ck=ck+kolr(i)*kolr(i)/dble(ns(i))
      END DO
      n=Ie-Ib+1
      Chikw=12D0*ck/(n*(n+1))-3*(n+1)
      ndf=Nyr-1
      P5=chisq(Chikw,ndf)*100D0
      IF(.not.Lprt.or.Lhiddn)RETURN
      sp1=0
      IF(Lwdprt)sp1=18
      xb='                                                  '
      IF(Lcmpaq)THEN
       WRITE(Mt1,1011)
 1011  FORMAT(/,'  Nonparametric Test for the Presence of Seasonality ',
     &       'Assuming Stability')
       WRITE(Mt1,1021)xb(1:(sp1+11))
 1021  FORMAT(/,a,'Kruskal-Wallis statistic',2x,'Dgrs.freedom',2x,
     &       'Probability level')
       WRITE(Mt1,1031)xb(1:(sp1+24)),Chikw,ndf,P5
 1031  FORMAT(a,F11.4,6X,I3,7X,F9.3,'%',/)
      ELSE
       WRITE(Mt1,1010)xb(1:(sp1+2))
 1010  FORMAT(//,a,'Nonparametric Test for the Presence of Seasonality',
     &       ' Assuming Stability')
       WRITE(Mt1,1020)xb(1:(sp1+17)),xb(1:(sp1+19))
 1020  FORMAT(/,a,'Kruskal-Wallis',6x,'Degrees of',4x,'Probability',/,
     &        a,'Statistic',8x,'Freedom',9x,'Level')
       WRITE(Mt1,1030)xb(1:(sp1+18)),Chikw,ndf,P5
 1030  FORMAT(/,a,F11.4,9X,I3,8X,F9.3,'%',/)
      END IF
      IF(P5.le.1D0)THEN
       WRITE(Mt1,1040)xb(1:(sp1+12))
 1040  FORMAT(a,'Seasonality present at the one percent level.')
       RETURN
      END IF
      WRITE(Mt1,1050)xb(1:(sp1+12))
 1050 FORMAT(a,'No evidence of seasonality at the one percent level.')
      RETURN
      END
