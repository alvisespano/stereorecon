/* 
 *  Copyright (c) 2008  Noah Snavely (snavely (at) cs.washington.edu)
 *    and the University of Washington
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

/* BaseApp.cpp */
/* Base application */

#include "BaseApp.h"

#include "defines.h"

std::vector<KeypointMatch>& MatchTable::GetMatchList(MatchIndex idx)
{
    AdjListElem e;
    e.m_index = idx.second;
    MatchAdjList &l = m_match_lists[idx.first];
    std::pair<MatchAdjList::iterator, MatchAdjList::iterator> p =
        equal_range(l.begin(), l.end(), e);

    /*printf("this->m_match_list[%ld]: (size = %d) \n", idx.first, l.size());
    for (size_t i = 0; i < l.size(); ++i)
    {
        const AdjListElem& ale = l[i];
        printf("  AdjListElem: m_index = %d, m_match_list: (size = %d) \n", ale.m_index, ale.m_match_list.size());
        for (size_t j = 0; j < ale.m_match_list.size(); ++j)
        {
            const KeypointMatch& kpm = ale.m_match_list[j];
            printf("    %d %d\n", kpm.m_idx1, kpm.m_idx2);
        }
    }

    if (p.first == l.end()) printf("first past end\n");
    else
    {
        const AdjListElem& f = *p.first;
        printf("first: m_index = %d, m_match_list = \n", f.m_index);
        for (size_t i = 0; i < f.m_match_list.size(); ++i)
        {
            const KeypointMatch& kpm = f.m_match_list[i];
            printf("  %d %d\n", kpm.m_idx1, kpm.m_idx2);
        }
    }
    if (p.second == l.end()) printf("second past end\n");
    else
    {
        const AdjListElem& s = *p.second;
        printf("second: m_index = %d, m_match_list = \n", s.m_index);
        for (size_t i = 0; i < s.m_match_list.size(); ++i)
        {
            const KeypointMatch& kpm = s.m_match_list[i];
            printf("  %d %d\n", kpm.m_idx1, kpm.m_idx2);
        }
    }*/

    assert(p.first != p.second); // l.end());

    return (p.first)->m_match_list;
}

Keypoint &BaseApp::GetKey(int img, int key) {
    return m_image_data[img].m_keys[key];
}

KeypointWithDesc &BaseApp::GetKeyWithDesc(int img, int key) {
    return m_image_data[img].m_keys_desc[key];
}

int BaseApp::GetNumKeys(int img) {
    return (int) m_image_data[img].m_keys.size();
}

/* Get the index of a registered camera */
int BaseApp::GetRegisteredCameraIndex(int cam) {
    int num_images = GetNumImages();

    int count = 0;
    for (int i = 0; i < num_images; i++) {
	if (m_image_data[i].m_camera.m_adjusted) {
	    if (count == cam)
		return i;
	    
	    count++;
	}
    }

    printf("[SifterApp::GetRegisteredCameraIndex] "
	   "Error: ran out of cameras\n");

    return -1;
}

/* Return the number of images */
int BaseApp::GetNumImages() {
    return (int) m_image_data.size();
}

/* Return the number of original images */
int BaseApp::GetNumOriginalImages() {
    return m_num_original_images;
}

int BaseApp::GetNumMatches(int i1, int i2) 
{
    int i_min = MIN(i1, i2);
    int i_max = MAX(i1, i2);
    
    MatchIndex idx = GetMatchIndex(i_min, i_max);

    // if (m_match_lists.find(idx) == m_match_lists.end())
    //    return 0; 
    // return m_match_lists[idx].size();

    return m_matches.GetNumMatches(idx);
}

#if 1
/* Get match information */
MatchIndex GetMatchIndex(int i1, int i2) {
    // MatchIndex num_images = GetNumImages();
    // return i1 * num_images + i2;
    return MatchIndex((unsigned long) i1, (unsigned long) i2);
}

MatchIndex GetMatchIndexUnordered(int i1, int i2) {
    // MatchIndex num_images = GetNumImages();
    // return i1 * num_images + i2;
    if (i1 < i2)
        return MatchIndex((unsigned long) i1, (unsigned long) i2);
    else
        return MatchIndex((unsigned long) i2, (unsigned long) i1);
}
#endif

void BaseApp::SetMatch(int i1, int i2) {
    m_matches.SetMatch(GetMatchIndex(i1, i2));
}

void BaseApp::RemoveMatch(int i1, int i2) {
    m_matches.RemoveMatch(GetMatchIndex(i1, i2));
}
    
bool BaseApp::ImagesMatch(int i1, int i2) {
    return m_matches.Contains(GetMatchIndex(i1, i2));
}

int BaseApp::FindImageWithName(const char *name)
{
    int num_images = GetNumImages();
    
    for (int i = 0; i < num_images; i++) {
	if (strcmp(m_image_data[i].m_name, name) == 0)
	    return i;
    }

    return -1;
}

void BaseApp::ReindexPoints() 
{
    int num_images = GetNumImages();

    int adjusted = 0;
    int *reindex = new int[num_images];

    m_num_views_orig.clear();

    for (int i = 0; i < num_images; i++) {
        if (m_image_data[i].m_camera.m_adjusted &&
            m_image_data[i].m_licensed) {
            reindex[i] = adjusted;
            adjusted++;
        }
    }

    int num_points = m_point_data.size();
    
    for (int i = 0; i < num_points; i++) {
        int num_views = (int) m_point_data[i].m_views.size();
        m_num_views_orig.push_back(num_views);

        for (int j = 0; j < num_views; j++) {   
            int v = m_point_data[i].m_views[j].first;

            if (!m_image_data[v].m_camera.m_adjusted ||
                !m_image_data[v].m_licensed) {
                m_point_data[i].m_views.
                    erase(m_point_data[i].m_views.begin() + j);
                j--;
                num_views--;
            } else {
                m_point_data[i].m_views[j].first = reindex[v];
            }
        }
    }

    delete [] reindex;
}
