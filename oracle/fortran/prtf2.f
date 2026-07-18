C     Last change:  BCM  16 Feb 1999    3:48 pm
      SUBROUTINE prtf2(Nw,Mqf2,Khcfm)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE GENERATES THE F2 TABLE for standard printouts.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'title.cmn'
      INCLUDE 'inpt2.cmn'
      INCLUDE 'work2.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'tests.cmn'
      INCLUDE 'mq3.cmn'
c-----------------------------------------------------------------------
      CHARACTER Mqf2*(7),aorb*(1)
      INTEGER i,Khcfm,n,n1,Nw
      DIMENSION aorb(2)
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      DATA aorb/'B','A'/
c-----------------------------------------------------------------------
      WRITE(Nw,1010)Pcdif(1:nblank(Pcdif)),aorb(Khcfm),Mqf2,Mqcd
 1010 FORMAT(3X,'F 2.A: Average ',a,' without regard to sign over the',
     &       /,10x,'indicated span',/,6X,'Span',/,7X,'in',6X,A1,'1',5X,
     &       'D11',5X,'D13',5X,'D12',5X,'D10',6X,'A2',5X,'D18',6X,'F1',
     &       /,3X,A7,'s',4X,'O',6X,'CI',7X,'I',7X,'C',7X,'S',7X,'P',5X,
     &       'TD&H',5X,A3)
      DO i=1,Ny
       WRITE(Nw,1020)i,Obar(i),Cibar(i),Ibar(i),Cbar(i),Sbar(i),Pbar(i),
     &               Tdbar(i),Smbar(i)
 1020  FORMAT(7X,I2,8F8.2)
      END DO
      WRITE(Nw,1030)Mqf2
 1030 FORMAT(/,6X,'Span',/,7X,'in',5X,'E1',6X,'E2',6X,'E3',/,3X,A7,'s',
     &       2X,'Mod.O   Mod.CI  Mod.I')
      DO i=1,Ny
       WRITE(Nw,1040)i,Ombar(i),Cimbar(i),Imbar(i)
 1040  FORMAT(7X,I2,3F8.2)
      END DO
c-----------------------------------------------------------------------
      WRITE(Nw,1050)Pcdif(1:nblank(Pcdif)),Mqf2
 1050 FORMAT(/,3X,'F 2.B: Relative contributions to the variance of the'
     &       ,a15,/,10x,'in the components of the original series',/,6x,
     &       'Span',/,7X,'in',5X,'E3',5X,'D12',5X,'D10',6X,'A2',5X,
     &       'D18',12X,'RATIO',/,3X,A7,'s',3X,'I',7X,'C',7X,'S',7X,'P',
     &       5X,'TD&H',4X,'TOTAL   (X100)')
      DO i=1,Ny
       WRITE(Nw,1060)i,Isq(i),Csq(i),Ssq(i),Psq(i),Tdsq(i),Osq2(i)
 1060  FORMAT(7X,I2,5(2PF8.2),'  100.00',2PF8.2)
      END DO
c-----------------------------------------------------------------------
      WRITE(Nw,1070)Pcdif(1:nblank(Pcdif)),aorb(Khcfm),Mqf2
 1070 FORMAT(/,3X,
     &       'F 2.C: Average ',A,' with regard to sign and standard',/,
     &       10x,'deviation over indicated span',/,6X,'Span',8X,A1,'1',
     &       15X,'D13',14X,'D12',/,7X,'in',10X,'O',16X,'I',16X,'C',/,
     &       3X,A7,'s',3(3X,'Avg.',4X,'S.D.',2X))
      DO i=1,Ny
       WRITE(Nw,1080)i,Obar2(i),Osd(i),Ibar2(i),Isd(i),Cbar2(i),Csd(i)
 1080  FORMAT(7X,I2,3(F9.2,F8.2))
      END DO
      WRITE(Nw,1090)Mqcd,Mqf2
 1090 FORMAT(/,6X,'Span',8X,'D10',14X,'D11',15X,'F1',/,7X,'in',10X,'S',
     &       16X,'CI',14X,A3,/,3X,A7,'s',3(3X,'Avg.',4X,'S.D.',2X))
      DO i=1,Ny
       WRITE(Nw,1080)i,Sbar2(i),Ssd(i),Cibar2(i),Cisd(i),Smbar2(i),
     &               Smsd(i)
      END DO
c-----------------------------------------------------------------------
      WRITE(Nw,1100)Mqcd,Adrci,Adri,Adrc,Adrmcd
 1100 FORMAT(/,3X,'F 2.D: Average duration of run',8X,'CI',6X,'I',7X,
     &       'C',6X,A3,/,36X,4F8.2)
c-----------------------------------------------------------------------
      IF(Ny.eq.12)Kpage=Kpage+1
c-----------------------------------------------------------------------
      WRITE(Nw,1110)Moqu(1:nblank(Moqu))
 1110 FORMAT(//,3X,'F 2.E: I/C Ratio for ',A,'s span')
      n=6
      IF(Ny.eq.4)n=Ny
      WRITE(NW,1120)(i,i=1,n)
 1120 FORMAT(/,6x,'SPAN  ',7I8)
      WRITE(NW,1130)(Smic(i),i=1,n)
 1130 FORMAT(6x,'I/C    ',6F8.2)
      IF(n.lt.Ny)THEN
       WRITE(NW,1120)(i,i=n+1,Ny)
       WRITE(Nw,1130)(Smic(i),i=n+1,Ny)
      END IF
c-----------------------------------------------------------------------
      WRITE(Nw,1140)Moqu(1:nblank(Moqu)),Mcd
 1140 FORMAT(/,7X,A7,'s for cyclical dominance:',i8)
c-----------------------------------------------------------------------
      WRITE(Nw,1150)Vi,Vc,Vs,Vp,Vtd,Rv
 1150 FORMAT(//,3X,'F 2.F: Relative contribution of the components to ',
     &       'the stationary',/,10x,'portion of the variance in the ',
     &       'original series',//,19x,'I',7x,'C',7X,'S',7X,'P',5X,
     &       'TD&H',3X,'Total',/,14X,6F8.2,/)
c-----------------------------------------------------------------------
      n=Ny+2
      WRITE(Nw,1160)n
 1160 FORMAT(/,3X,'F 2.G: The autocorrelation of the irregulars for ',
     &       'spans 1 to',I3)
      n1=n
      IF(Ny.eq.12)n1=7
      WRITE(Nw,1120)(i,i=1,n1)
      WRITE(Nw,1170)(Autoc(i),i=1,n1)
 1170 FORMAT(6x,'ACF    ',7f8.2)
      IF(n1.lt.n)THEN
       WRITE(Nw,1120)(i,i=n1+1,n)
       WRITE(Nw,1170)(Autoc(i),i=n1+1,n)
      END IF
c-----------------------------------------------------------------------
      WRITE(Nw,1180)Ratic
 1180 FORMAT(/,3X,'F 2.H: The final I/C Ratio from Table D12:',F12.2)
      IF(Kfulsm.lt.2)WRITE(Nw,1181)Ratis
 1181 FORMAT(9X,' The final I/S Ratio from Table D10:',F12.2)
c-----------------------------------------------------------------------
      WRITE(Nw,1190)Fpres,P3
 1190 FORMAT(/,3X,'F 2.I:',52X,'Statistic   Prob.',/,73x,'level',/,
     &       4X,'F-test for stable seasonality from Table B 1.',8x,':',
     &       F11.3,F8.2,'%')
c      IF(Kdwopt.ge.1.and.Kdwopt.lt.6)WRITE(Nw,1170)F(4),P(4)
c 1170 FORMAT(4X,'F-test for the trading day regession in Table C15.',
c     &       2X,':',F11.3,F8.2,'%')
      WRITE(Nw,1200)Fstabl,P1,Chikw,P5,Fmove,P2
 1200 FORMAT(4X,'F-test for stable seasonality from Table D 8.',8X,':',
     &       F11.3,F8.2,'%',/,4X,'Kruskal-Wallis Chi Squared test',/,
     &       18x,'for stable seasonality from Table D 8. :',F11.3,F8.2,
     &       '%',/,4X,'F-test for moving seasonality from Table D 8.',
     &       8X,':',F11.3,F8.2,'%',/)
c-----------------------------------------------------------------------
      RETURN
      END
