C     Last change:  BCM  19 Apr 2007    4:06 pm
**==wrttbl.f    processed by SPAG 4.03F  at 09:55 on  1 Mar 1994
      SUBROUTINE wrttbl(Tmp,Jyr,Tyrly,L,Kdec,Mt1,Tblfmt,Tblwid,Tblcol,
     &                  Disp1,Disp2,Disp3,Nb,Nb1,Ipow,Lastcl)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C THIS SUBROUTINE WAS ORIGINALLY WRITTEN BY DAVE PALETZ OF SRD, 9/91
C REVISED: BRIAN MONSELL, 8/95
c-----------------------------------------------------------------------
C SOMETIMES ONE OR MORE MONTHS IN A YEAR HAVE NO DATA PROVIDED BY THE
C USER.  WHEN SO, THE PROGRAM DISPLAYS AN ERROR MESSAGE WHEN IT PRINTS
C THESE BLANK FIGURES IN THE TABLE AS REAL NUMBERS.  THIS WAS INTENDED
C SO THAT THE ASTERISKS WOULD REMIND THE USER THAT DATA WAS MISSING FOR
C THAT MONTH.  THE SUBROUTINE BELOW SUPRESSES THE ERROR MESSAGE WHILE
C CONTINUING TO PLACE THE ASTERISKS WHERE THEY ARE NEEDED.
C
C I:       Looping variable
C IPOW:    0 if figure should not be expressed as percentage; 1 if it
C          should be
C JYR:     Year of the data to print
C KDEC:    Number of places behind decimal to display
C L:       Number of observations for the year plus total (12+1=13 for
C          monthly figures; 4+1=5 for quarterly)
C LOOP:    Looping variable
C MT1:     The unit number of the output file
C TBLFMT:  The format to use in the table
C TMP:     The user supplied observations for one year
C TYRLY:   Title for the average or total output line
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      DOUBLE PRECISION BIG
      PARAMETER(BIG=10D16)
c-----------------------------------------------------------------------
      DOUBLE PRECISION Tmp,postmp
      LOGICAL Lstar,Lastcl
      INTEGER i,Jyr,Kdec,L,Tblcol,loop,Mt1,Tblwid,Ipow,lstob,ipos,j,j2,
     &        Disp1,Disp2,Disp3,cnum,nlen,Nb,Nb1,ipos2
      CHARACTER Tblfmt*(*),Tyrly*(5),chtmp*(25),star*(25),blank*(56),
     &          dfmt*(10),xlin*(132),cy*(4)
      DIMENSION chtmp(PSP+1),Tmp(*),postmp(PSP+1)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      DOUBLE PRECISION ceilng
      EXTERNAL dpeq,ceilng
c-----------------------------------------------------------------------
      DATA star/'*************************'/
      DATA blank/
     &     '                                                        '/
c-----------------------------------------------------------------------
c     Set position of last observation of the year
c-----------------------------------------------------------------------
      lstob=L
      IF(Lastcl)lstob=lstob-1
c-----------------------------------------------------------------------
C	Go through L elements one at a time checking to see if special
c     output is needed
c-----------------------------------------------------------------------
      Lstar=.false.
      DO i=1,L
c-----------------------------------------------------------------------
C	If the element is 1 quadrillion or more, this means the 
C     observation is assumed to be missing for this month/quarter.  
c     Set Lstar to true, and set up the label for missing values.
c     If the missing value is before the start or end of the series,
c     set the label to blanks instead of stars.
c-----------------------------------------------------------------------
       chtmp(i)=blank(1:Tblwid+2)
       IF(dpeq(Tmp(i),DNOTST).or.Tmp(i).ge.BIG)THEN
        IF(.not.Lstar)Lstar=.true.
        chtmp(i)=star(1:Tblwid+2)
        IF(Nb.gt.0.and.i.lt.Nb)chtmp(i)=blank(1:Tblwid+2)
        IF(Nb1.gt.0.and.i.gt.Nb1)chtmp(i)=blank(1:Tblwid+2)
       ELSE
        postmp(i)=Tmp(i)
        IF(Ipow.eq.1)postmp(i)=postmp(i)*100d0
c-----------------------------------------------------------------------
c     For cases where Kdec = 0 and the decimal fraction is exactly .5,
c     make an adjustment to ensure the number will round properly
c     when printed (BCM April 2007)
c-----------------------------------------------------------------------
        IF(dpeq(postmp(i)-ceilng(postmp(i)-0.5D0),0.5D0).and.Kdec.eq.0)
     &     postmp(i)=postmp(i)+0.01D0
       END IF
      END DO
      IF(Lstar)THEN
c-----------------------------------------------------------------------
c     Initial variables for printing stars for missing values.
c-----------------------------------------------------------------------
       loop=lstob/Tblcol
       IF(mod(lstob,Tblcol).gt.0)loop=loop+1
       cnum=1
c-----------------------------------------------------------------------
c     Write the observation format into dfmt.
c-----------------------------------------------------------------------
       IF(Tblwid.le.9)THEN
        WRITE(dfmt,1010)Disp2,Tblwid,Kdec
 1010   FORMAT('(',i1,'x,f',i1,'.',i1,')')
       ELSE
        WRITE(dfmt,1020)Disp2,Tblwid,Kdec
 1020   FORMAT('(',i1,'x,f',i2,'.',i1,')')
       END IF
c-----------------------------------------------------------------------
c     For each line to be printed out, set up a character string
c     in the table format.
c-----------------------------------------------------------------------
       DO i=1,loop
c-----------------------------------------------------------------------
c     IF this is the first line, store year or label first on xlin.
c-----------------------------------------------------------------------
        IF(i.eq.1)THEN
         IF(Tyrly.eq.'XXXXX')THEN
          nlen=1
          CALL itoc(Jyr,cy,nlen)
          IF(Lfatal)RETURN
          xlin(1:(Disp1+6))=blank(1:(7-nlen))//cy(1:(nlen-1))//
     &                      blank(1:Disp1)
         ELSE
          xlin(1:(Disp1+6))=' '//Tyrly//blank(1:Disp1)
         END IF
c-----------------------------------------------------------------------
c     For all other lines, store blanks for first Disp1+6 spaces of the
c     line.
c-----------------------------------------------------------------------
        ELSE
         xlin(1:(Disp1+6))=blank(1:(Disp1+6))
        END IF
        ipos=Disp1+6
c-----------------------------------------------------------------------
c     Loop through all the observations that will be printed on this
c     line.  
c-----------------------------------------------------------------------
        j2=cnum+Tblcol-1
        IF(j2.gt.lstob)j2=lstob
        DO j=cnum,j2
c-----------------------------------------------------------------------
c     If observation is considered missing, store out stars; else, 
c     write the observation in the correct format.
c-----------------------------------------------------------------------
         ipos2=ipos+Tblwid+Disp2
         IF(dpeq(Tmp(j),DNOTST).or.Tmp(j).ge.BIG)THEN
          IF(Disp2.eq.0)THEN
           xlin((ipos+1):ipos2)=chtmp(j)(1:Tblwid)
          ELSE
           xlin((ipos+1):ipos2)=blank(1:Disp2)//chtmp(j)(1:Tblwid)
          END IF
         ELSE
          WRITE(xlin((ipos+1):ipos2),dfmt)postmp(j)
         END IF
         ipos=ipos2
        END DO
        IF(i.eq.loop.and.L.gt.lstob.and.Disp3.gt.0)THEN
         xlin((ipos+1):(ipos+Disp3))=blank(1:Disp3)
         ipos=ipos+Disp3
         WRITE(dfmt,1030)Tblwid+2,Kdec
 1030    FORMAT('(   f',i2,'.',i1,')')
         WRITE(xlin((ipos+1):(ipos+Tblwid+2)),dfmt)postmp(j)
         ipos=ipos+Tblwid+2
        END IF
c-----------------------------------------------------------------------
c     Write fully formatted line to output file
c-----------------------------------------------------------------------
        WRITE(Mt1,1040)xlin(1:ipos)
 1040   FORMAT(a)
        cnum=cnum+Tblcol
       END DO
      ELSE
c-----------------------------------------------------------------------
C	Print in table format.
c-----------------------------------------------------------------------
       IF(Tyrly.eq.'XXXXX')THEN
        WRITE(Mt1,Tblfmt)Jyr,(postmp(j),j=1,L)
       ELSE
        WRITE(Mt1,Tblfmt)Tyrly,(postmp(j),j=1,L)
       END IF
      END IF
      RETURN
      END
