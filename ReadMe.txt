	/*pruning begin*/
		for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
			int u1 = *it;
			bool has = false;
			for (auto it2 = group_items[(dir)]->begin(); it2 != group_items[(dir)]->end(); it2++){
				int u2 = *it2;
				//cout << v << " to label " << label[OPP(dir)] << " dist "<< e_dist << endl;
				int e_dist = EstimatDist(u1, u2);
				if (e_dist <= delta_){
					has = true;
					break;
				}
			}

			if (false == has){
				group_items[(dir)]->erase(it++);
				total++;
				flag = true;
			}
			else
			{
				it++;
			}
		}

void TestTimer(){
	LARGE_INTEGER  large_interger;
	double dff;
	__int64  c1, c2;
	QueryPerformanceFrequency(&large_interger);
	dff = large_interger.QuadPart;
	QueryPerformanceCounter(&large_interger);
	c1 = large_interger.QuadPart;
	Sleep(800);
	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("本机高精度计时器频率%lf\n", dff);
	printf("第一次计时器值%I64d 第二次计时器值%I64d 计时器差%I64d\n", c1, c2, c2 - c1);
	printf("计时%lf毫秒\n", (c2 - c1) * 1000 / dff);

	printf("By MoreWindows\n");
}



#if 0
		END_FOR_EACH_DIRCTION;
		NewSupport *s[2];
		CountNew *c[2];
		s[0] = &supports_[0][u]; //left
		s[1] = &supports_[1][u]; //right
		c[0] = &counts_new_[0][u];   //left	
		c[1] = &counts_new_[1][u];   //right
		int label = g_->vertex(u)->label;

		/*????????????????????????????????here have some problems.*/
		FOR_EACH_DIRECTION;
		for (auto it = s[dir]->begin(); it != s[dir]->end(); it++){
			int s_vertex = *it; // dir = 0, out support
			int s_label = g_->vertex(s_vertex)->label; //find group by label
			bool flag = false;
			if (c[OPP(dir)]->find(label) != c[OPP(dir)]->end()){
				c[OPP(dir)]->at(label)--; //counter ;
				if (0 >= c[OPP(dir)]->at(s_label)){
					flag = true;
				}
			}
			else
			{
				flag = true;
			}

			//count not exist or 0
			if (true == flag){
				int l = g_->vertex(s_vertex)->label;
				group_->items(l)->erase(s_vertex);
				deleted_queue_.push(s_vertex);
				count++;
			}
		}
		END_FOR_EACH_DIRCTION;
	}
#endif