#ifndef __Animation_h_
#define __Animation_h_

#include <list>
#include <stdio.h>
#include <limits>

template< class KeyType >
class Animation
{

public:
	
	typedef typename std::list< KeyType > KeyCont;
	typedef typename KeyCont::iterator KeyContIt;
    typedef typename KeyCont::reverse_iterator KeyContRevIt;

	Animation() {}
	~Animation() {}
	
    int GetNumberOfKeys() { return m_keys.size(); }

	bool HasKey( int frame )
	{
		KeyContIt it = m_keys.begin();
		while( it != m_keys.end() )
		{
			if( it->frame == frame )
			{
				return true;
			}
			++it;
		}
		return false;
	}

    int FindClosestKey( int frame )
    {
        int closestKeyFrame = -1;

        KeyContIt it = m_keys.begin();
        int minDiff = std::numeric_limits<int>::max();
        while( it != m_keys.end() )
        {
            if( abs( it->frame - frame ) < minDiff )
            {
                closestKeyFrame = it->frame;
                minDiff = abs( it->frame - frame );
            }
            ++it;
        }
        return closestKeyFrame;
    }

    int GetFrameForKey( int keyIndex )
    {
        KeyContIt it = m_keys.begin();
        int i = 0;
        while( it != m_keys.end() )
        {
            if( i == keyIndex )
                return it->frame;
            ++it;
            ++i;
        }
        return -1;
    }

    int GetKeyForFrame( int frame )
    {
        KeyContIt it = m_keys.begin();
        int keyIndex = 0;
        while( it != m_keys.end() )
        {
            if( it->frame == frame )
            {
                return keyIndex;
            }
            ++it;
            ++keyIndex;
        }
        return -1;
    }

	void AddKey( const KeyType & newKey )
	{
		KeyContIt it = m_keys.begin();
		bool replace = false;
		while( it != m_keys.end() )
		{
			if( it->frame >= newKey.frame )
			{
				if( it->frame == newKey.frame )
					replace = true;
				break;
			}
			++it;
		}
		
		if( replace )
			(*it) = newKey;
		else
			m_keys.insert( it, newKey );
	}

    void RemoveKey( int frame )
    {
        KeyContIt it = m_keys.begin();
        while( it != m_keys.end() )
        {
            if( it->frame == frame )
                m_keys.erase( it );
            if( it->frame >= frame )
                break;
            ++it;
        }
    }

    virtual void MoveKey( int oldFrame, int newFrame )
    {
        KeyContIt it = m_keys.begin();
        while( it != m_keys.end() )
        {
            if( it->frame == oldFrame )
            {
                it->frame = newFrame;
                KeyContIt itNext = it;
                ++itNext;
                if( itNext != m_keys.end() && itNext->frame <= it->frame )
                    it->frame = itNext->frame - 1;
                if( it != m_keys.begin() )
                {
                    KeyContIt itPrev = it;
                    --itPrev;
                    if( itPrev->frame >= it->frame )
                        it->frame = itPrev->frame + 1;
                }
            }
            if( it->frame >= oldFrame )
                break;
            ++it;
        }
    }

    int NextKeyframe( int frame )
    {
        KeyContIt it = m_keys.begin();
        while( it != m_keys.end() )
        {
            if( it->frame > frame )
                return it->frame;
            ++it;
        }
        return -1;
    }

    int PrevKeyframe( int frame )
    {
        KeyContRevIt it = m_keys.rbegin();
        while( it != m_keys.rend() )
        {
            if( it->frame < frame )
                return it->frame;
            ++it;
        }
        return -1;
    }
	
    virtual bool ComputeFrame( int frame, KeyType & key )
	{
        if( m_keys.size() == 0 )
            return false;

		KeyContIt itCur = m_keys.begin();
		KeyContIt itNext = m_keys.end();
		itNext--;

		// Only one key, use that one
		if( itCur == itNext )
		{
			key = *itCur;
		}
		// on first key
		else if( frame <= itCur->frame )
		{
			key = *itCur;
		}
		// on last key
		else if( frame >= itNext->frame )
		{
			key = *itNext;
		}
		// between first and last key
		else if( frame > itCur->frame && frame < itNext->frame )
		{
			KeyContIt it = itCur;
			while( frame > it->frame )
				++it;
			itNext = it;
			itCur = it;
			--itCur;

			double ratio =  ((double)( frame - itCur->frame )) / ( itNext->frame - itCur->frame );
			
			// Interpolate data
			key.Interpolate( *itCur, *itNext, ratio );
		}

        return true;
	}
	
	bool IsEmpty() { return (m_keys.size() == 0); }

protected:

	KeyCont m_keys;

};


#endif
