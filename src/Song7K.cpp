#include "Global.h"
#include "Song.h"
#include <algorithm>

Song7K::Song7K()
{
	MeasureLength = 4;
	LeadInTime = 1.5;
	UseSeparateTimingData = false;
}

Song7K::~Song7K()
{
}

int tSort(const SongInternal::TimingSegment &i, const SongInternal::TimingSegment &j)
{
	return i.Time < j.Time;
}

void Song7K::Process()
{
	/* 
		We'd like to build the notes' position from 0 to infinity, 
		however the real "zero" position would be the judgement line
		in other words since "up" is negative relative to 0
		and 0 is the judgement line
		position would actually be
		judgeline - positiveposition
		and positiveposition would just be
		measure * measuresize + fraction * fractionsize
	*/

	/* For all difficulties */
	for (std::vector<SongInternal::TDifficulty<TrackNote>*>::iterator Diff = Difficulties.begin(); Diff != Difficulties.end(); Diff++)
	{
		if (!(*Diff)->Timing.size())
			continue;

		/* For all channels of this difficulty */
		for (int KeyIndex = 0; KeyIndex < (*Diff)->Channels; KeyIndex++)
		{
			glm::vec2 BasePosition (KeyIndex * (GearWidth / (*Diff)->Channels), 0);
			int MIdx = 0;

			/* For each measure of this channel */
			for (std::vector<SongInternal::Measure<TrackNote>>::iterator Measure = (*Diff)->Measures[KeyIndex].begin(); 
				Measure != (*Diff)->Measures[KeyIndex].end();
				Measure++)
			{
				/* For each note in the measure... */
				for (uint32 Note = 0; Note < Measure->MeasureNotes.size(); Note++)
				{
					/* 
					    Calculate position. (Change this to TrackNote instead of processing?)
					    issue is not having the speed change data there.
					*/
					TrackNote &CurrentNote = (*Measure).MeasureNotes[Note];
					int NoteMeasure = CurrentNote.GetMeasure();
					float MeasureVerticalD = MeasureBaseSpacing * NoteMeasure;
					float FractionVerticalD = 1.0f / float((*Diff)->Measures[KeyIndex][NoteMeasure].Fraction) * float(CurrentNote.GetFraction()) * MeasureBaseSpacing;
					glm::vec2 VerticalPosition( 0, MeasureVerticalD + FractionVerticalD );

					// if upscroll change minus for plus as well as matrix at screengameplay7k
					CurrentNote.AssignPosition(BasePosition - VerticalPosition);
				}
				MIdx++;
			}
		}

		/* 
			Now, vertical speeds. 
			The model says we have to move a measure in a certain time
			this certain time is equal to spb * mlen
			so since v = d/t we'd have
		*/

		/* Calculate VSpeeds. */
		for(TimingData::iterator Time = (*Diff)->Timing.begin();
			Time != (*Diff)->Timing.end();
			Time++)
		{
			SongInternal::TimingSegment VSpeed;
			float TValue = Time->Value;
			float FTime = (spb (Time->Value) * (float)MeasureLength);
			VSpeed.Time = TimeAtBeat((*Diff)->Timing, (*Diff)->Offset, Time->Time) + StopTimeAtBeat((*Diff)->StopsTiming, Time->Time);
			VSpeed.Value = MeasureBaseSpacing / FTime;
			(*Diff)->VerticalSpeeds.push_back(VSpeed);
		}

		for(TimingData::iterator Time = (*Diff)->StopsTiming.begin();
			Time != (*Diff)->StopsTiming.end();
			Time++)
		{
			SongInternal::TimingSegment VSpeed;
			float TValue = TimeAtBeat((*Diff)->Timing, (*Diff)->Offset, Time->Time) + StopTimeAtBeat((*Diff)->StopsTiming, Time->Time);
			float TValueN = TimeAtBeat((*Diff)->Timing, (*Diff)->Offset, Time->Time) + StopTimeAtBeat((*Diff)->StopsTiming, Time->Time) + Time->Value;

			/* Initial Stop */
			VSpeed.Time = TValue;
			VSpeed.Value = 0;

			/* First, eliminate collisions. */

			for (TimingData::iterator k = (*Diff)->VerticalSpeeds.begin(); k != (*Diff)->VerticalSpeeds.end(); k++)
			{
				if ( abs(k->Time - TValue) < 0.000001 ) /* Too close? Remove the collision, leaving only the 0 in front. */
				{
					k = (*Diff)->VerticalSpeeds.erase(k);

					if (k == (*Diff)->VerticalSpeeds.end())
						break;
				}
			}

			(*Diff)->VerticalSpeeds.push_back(VSpeed);

			bool AddTiming = true;

			/* Restored speed after stop */
			VSpeed.Time = TValueN;
			VSpeed.Value = MeasureBaseSpacing / (spb(BpmAtBeat((*Diff)->Timing, Time->Time)) * MeasureLength);

			/* Check for collisions again, but prioritizing the BPM change over the stop. */
			for (TimingData::iterator k = (*Diff)->VerticalSpeeds.begin(); k != (*Diff)->VerticalSpeeds.end(); k++)
			{
				if ( abs(k->Time - TValueN) < 0.000001 )
				{
					AddTiming = false;
					break;
				}
			}

			if (AddTiming)
				(*Diff)->VerticalSpeeds.push_back(VSpeed);
		}

		std::sort((*Diff)->VerticalSpeeds.begin(), (*Diff)->VerticalSpeeds.end(), tSort);
	}
}