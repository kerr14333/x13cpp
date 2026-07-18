      SUBROUTINE ssfnot(Nopt,Nop2,Fnotvc,Fnotky,Nssky)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c    create footnotes for full printout of sliding spans -
c    this will create an integer vector of footnote numbers (Fnotvc)
c    an integer vector of unique footnote codes which can be used to
c    generate the key for the table (Fnotky) with the number of unique
c    footnotes for the table (Nssky).
c    Written by BCM - December 2006
c-----------------------------------------------------------------------
      INTEGER PSSTP,PSSSC,PSSNT
      PARAMETER(PSSTP=5,PSSSC=6,PSSNT=7)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssap.cmn'
      INCLUDE 'sspvec.cmn'
c-----------------------------------------------------------------------
      CHARACTER Fnotvc*(10)
      INTEGER Fnotky,Nopt,Nop2,Nssky,i
      DIMENSION Fnotvc(MXLEN),Fnotky(PSSNT)
c-----------------------------------------------------------------------
c     Initialize Fnotvc to zero, Fnotky to NOTSET
c-----------------------------------------------------------------------
      CALL setint(NOTSET,PSSNT,Fnotky)
      Nssky=0
c-----------------------------------------------------------------------
c     For each observation, check to see if any of the indicator
c     variables has determined whether the observation was flagged as
c     an extreme (Per), changed direction (Csign), or indicated a
c     turning point (Cturn)
c-----------------------------------------------------------------------
      DO i=Im,Sslen+Im-1
       CALL setchr(' ',10,Fnotvc(i))
       IF(Per(i,Nopt).eq.-1)THEN
        Fnotvc(i)='    NT    '
        IF(Fnotky(PSSNT).eq.NOTSET)THEN
         Fnotky(PSSNT)=1
         Nssky=Nssky+1
        END IF
       ELSE IF(Per(i,Nopt).gt.0)THEN
        IF(Fnotky(Per(i,Nopt)).eq.NOTSET)THEN
         Fnotky(Per(i,Nopt))=1
         Nssky=Nssky+1
        END IF
        IF((Cturn(i,Nopt).eq.1).and.(Csign(i,Nopt).eq.1).and.
     &      Per(i,Nopt).gt.0)THEN
         IF(Nop2.gt.0)THEN
          WRITE(Fnotvc(i),1010)'SC',Per(i,Nopt),Ch(Nopt)
         ELSE
          WRITE(Fnotvc(i),1010)'IE',Per(i,Nopt),Ch(Nopt)
         END IF
 1010    FORMAT(a,', TP, ',i1,a1)
         IF(Fnotky(PSSSC).eq.NOTSET)THEN
          Fnotky(PSSSC)=1
          Nssky=Nssky+1
         END IF
         IF(Fnotky(PSSTP).eq.NOTSET)THEN
          Fnotky(PSSTP)=1
          Nssky=Nssky+1
         END IF
        ELSE IF((Csign(i,Nopt).eq.1).and.Per(i,Nopt).gt.0)THEN
         IF(Nop2.gt.0)THEN
          WRITE(Fnotvc(i),1020)'SC',Per(i,Nopt),Ch(Nopt)
         ELSE
          WRITE(Fnotvc(i),1020)'IE',Per(i,Nopt),Ch(Nopt)
         END IF
 1020    FORMAT('  ',a2,', ',i1,a1,'  ')
         IF(Fnotky(PSSSC).eq.NOTSET)THEN
          Fnotky(PSSSC)=1
          Nssky=Nssky+1
         END IF
        ELSE IF((Cturn(i,Nopt).eq.1).and.Per(i,Nopt).gt.0)THEN
         WRITE(Fnotvc(i),1030)Per(i,Nopt),Ch(Nopt)
 1030    FORMAT('  TP, ',i1,a1,'  ')
         IF(Fnotky(PSSTP).eq.NOTSET)THEN
          Fnotky(PSSTP)=1
          Nssky=Nssky+1
         END IF
        ELSE IF(Per(i,Nopt).gt.0)THEN
         WRITE(Fnotvc(i),1040)Per(i,Nopt),Ch(Nopt)
 1040    FORMAT('    ',i1,a1,'    ')
        END IF
       ELSE IF((Cturn(i,Nopt).eq.1).and.(Csign(i,Nopt).eq.1)) THEN
        IF(Nop2.gt.0)THEN
         Fnotvc(i)='  SC, TP  '
        ELSE
         Fnotvc(i)='  IE, TP  '
        END IF
        IF(Fnotky(PSSSC).eq.NOTSET)THEN
         Fnotky(PSSSC)=1
         Nssky=Nssky+1
        END IF
        IF(Fnotky(PSSTP).eq.NOTSET)THEN
         Fnotky(PSSTP)=1
         Nssky=Nssky+1
        END IF
       ELSE IF(Cturn(i,Nopt).eq.1)THEN
        Fnotvc(i)='    TP    '
        IF(Fnotky(PSSTP).eq.NOTSET)THEN
         Fnotky(PSSTP)=1
         Nssky=Nssky+1
        END IF
       ELSE IF(Csign(i,Nopt).eq.1)THEN
        IF(Nop2.gt.0)THEN
         Fnotvc(i)='    SC    '
        ELSE
         Fnotvc(i)='    IE    '
        END IF
        IF(Fnotky(PSSSC).eq.NOTSET)THEN
         Fnotky(PSSSC)=1
         Nssky=Nssky+1
        END IF
       END IF
      END DO
c-----------------------------------------------------------------------
      RETURN
      END
      