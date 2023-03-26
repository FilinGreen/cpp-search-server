#pragma once


template <typename It>
class IteratorRange{
public:

 
 IteratorRange(It dbeg,It dend){
 it_begin_=dbeg;
 it_end_=dend;
 }
 
 It begin(){
 return it_begin_;
 }
 
 It end(){
 return it_end_;
 }
 
 size_t size(){
 return distance(it_begin_,it_end_);
 }
  
private:
It it_begin_;
It it_end_;
};

template <typename Iterator>
class Paginator {
public:
Paginator(Iterator bg, Iterator en, size_t size){

size_t sizer=distance(bg,en);
if(sizer>size){
size_t lists=1;
  if(sizer%size==0){
  lists=sizer/size;
  }else{lists=(sizer/size)+1;}
  
  Iterator listbegin=bg;
  Iterator listend=bg+size;
  
  for(size_t i=0;i<lists;++i){

  pages_.push_back({listbegin,listend});
  listbegin+=size;
  listend+=size;
  
  }
}else{pages_.push_back({bg,en});}
}

auto begin() const {
return pages_.begin();
 }

auto end() const{
return pages_.end();
}

private:
std::vector<IteratorRange<Iterator>> pages_;
}; 