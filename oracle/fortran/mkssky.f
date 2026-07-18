      SUBROUTINE mkssky(Fnotky,Nssky,Nopt,Nop2)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Generate key for sliding spans table
c-----------------------------------------------------------------------
      INTEGER PSSFL1,PSSFL3,PSSFL4,PSSTP,PSSSC,PSSNT
      PARAMETER(PSSFL1=1,PSSFL3=3,PSSFL4=4,PSSTP=5,PSSSC=6,PSSNT=7)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssap.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
c-----------------------------------------------------------------------
      INTEGER Fnotky,Nssky,Nopt,Nop2,i
      DIMENSION Fnotky(7)
c-----------------------------------------------------------------------
c    Print an explanation for each footnote
c-----------------------------------------------------------------------
      IF(Fnotky(PSSNT).eq.1)THEN
       WRITE(Mt1,1010)
 1010  FORMAT('    NT - Observation not included in sliding spans ',
     &        'comparisons.',/)
      END IF
c-----------------------------------------------------------------------
      IF(Fnotky(PSSSC).eq.1)THEN
       IF(Nop2.gt.0)THEN
        WRITE(Mt1,1070)
 1070   FORMAT('    SC - A sign change can be found for this ',
     &         'observation.',/)
       ELSE
        WRITE(Mt1,1071)
 1071   FORMAT('    IE - The estimates of this effect are ',
     &         'inconsistent for this observation;',
     &       /,'         one span indicates that the effect causes ',
     &         'an increase in the ',
     &       /,'         observed value, another that it causes a ',
     &         'decrease.',/)
       END IF
      END IF
c-----------------------------------------------------------------------
      IF(Fnotky(PSSTP).eq.1)THEN
       WRITE(Mt1,1100)
 1100  FORMAT('    TP - Span values for this observation have a ',
     &        'turning point.',/)
      END IF
c-----------------------------------------------------------------------
      DO i=PSSFL1,PSSFL3
       IF(Fnotky(i).gt.0)THEN
        WRITE(Mt1,1020)i,Ch(Nopt),Cut(Nopt,i),Cut(Nopt,i+1)
 1020   FORMAT('    ',i1,a1,' - The maximum percentage difference is ',
     &         'greater than or equal to ',f4.1,'%',/,
     &         '         but less than ',f4.1,'%.',/)
       END IF
      END DO
      IF(Fnotky(PSSFL4).eq.1)THEN
       WRITE(Mt1,1030)PSSFL4,Ch(Nopt),Cut(Nopt,PSSFL4)
 1030  FORMAT('    ',i1,a1,' - The maximum percentage difference is ',
     &        'greater than or equal to ',f4.1,'%.',/)
      END IF
c-----------------------------------------------------------------------
      WRITE(Mt1,1110)
 1110 FORMAT(' ')
c-----------------------------------------------------------------------
      RETURN
      END
      