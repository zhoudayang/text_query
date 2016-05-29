#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include<algorithm>
using namespace std;

class TextQuery;
class Query;
class Query_base;
class WordQuery;
class NotQuery;
class BinaryQuery;
class AddQuery;
class OrQuery;

class TextQuery
{
public:
    typedef vector<string>::size_type line_no;
    typedef std::string::size_type str_size;
    void read_file(ifstream &is)
    {
        //将文件内容转储到vector中
        store_file(is);
        //建立键值对,键为单词,值为单词所在行号组成的set
        build_map();
    }

    set<line_no> run_query(const string&) const;
    string text_line(line_no) const;
    str_size size() const
    {
        return lines_of_text.size();
    }

private:
    void store_file(ifstream&);
    void build_map();
    std::vector<string> lines_of_text;
    std::map<string, set<line_no>> word_map;
};

string make_plural(size_t ctr,const string &word,const string &ending)
{
    return (ctr==1)?word:word+ending;
}
//输出结果
void print_results(const set<TextQuery::line_no>& locs, const TextQuery &file)
{
    // report no matches
    if (locs.empty()) {
        cout << "\nSorry. There are no entries for your query."
        << "\nTry again." << endl;
        return;
    }

    // if the word was found, then print count and all occurrences
    set<TextQuery::line_no>::size_type size = locs.size();
    cout << "match occurs "
    << size << (size == 1 ? " time:" : " times:") << endl;

    // print each line in which the word appeared
    set<TextQuery::line_no>::const_iterator it = locs.begin();
    for ( ; it != locs.end(); ++it) {
        cout << "\t(line "
        // don't confound user with text lines starting at 0
        << (*it) + 1 << ") "
        << file.text_line(*it) << endl;
    }
}
//打开文件
ifstream& open_file(ifstream &in, const string &file)
{
    in.close();
    in.clear();
    in.open(file.c_str());
    return in;
}
//将文件转储到vector中
void TextQuery::store_file(ifstream &is)
{
    string textline;
    while(getline(is,textline))
        lines_of_text.push_back(textline);
}
//建立键值对
void TextQuery::build_map()
{
    for(line_no line_num=0;line_num!=lines_of_text.size();++line_num)
    {
        istringstream line(lines_of_text[line_num]);
        string word;
        while(line>>word)
        {
            word_map[word].insert(line_num);
        }
    }
}
//执行查询
set<TextQuery::line_no> TextQuery::run_query(const string &query_word) const
{
    map<string,set<line_no> >::const_iterator loc=word_map.find(query_word);

    if(loc==word_map.end())
        return set<line_no>();
    else
        return loc->second;
}
//返回line对应的行
string TextQuery::text_line(line_no line) const
{
    if(line<lines_of_text.size())
        return lines_of_text[line];
    throw out_of_range("line number out of range");
}

class Query_base
{
    friend class Query;
protected:
    typedef TextQuery::line_no line_no;
    virtual ~Query_base(){}
private:
    //执行查询
    virtual std::set<line_no> eval(const TextQuery&) const=0;
    //显示查询结果
    virtual std::ostream& display(std::ostream& = std::cout) const=0;
};

class Query
{
    friend Query operator~(const Query &);
    friend Query operator|(const Query &,const Query &);
    friend Query operator&(const Query &,const Query &);

public:
    Query(const std::string&);
    //复制构造函数,复制对象,并更新引用计数的值
    Query(const Query &c):q(c.q),use(c.use)
    {
        ++*use;
    }
    //析构函数
    ~Query()
    {
        decr_use();
    }

    Query& operator=(const Query&);
    //执行查询
    std::set<TextQuery::line_no> eval(const TextQuery &t) const
    {
        return q->eval(t);
    }
    //向流os中输出查询到的结果
    std::ostream &display(std::ostream &os) const
    {
        return q->display(os);
    }
private:
    //一般构造函数
    Query(Query_base *query):q(query),use(new std::size_t(1))
    {
    }
    //Query_base 对象
    Query_base *q;
    //引用计数
    std::size_t *use;
    //减少引用计数的值,若引用计数的值为0,则删除对象
    void decr_use()
    {
        if(--*use==0)
        {
            delete q;
            delete use;
        }
    }
};
//重载运算符=
inline Query& Query::operator=(const Query &rhs)
{
    ++*rhs.use;
    decr_use();
    q = rhs.q;
    use = rhs.use;
    return *this;
}
//重载运算符 <<
inline std::ostream& operator<<(std::ostream &os,const Query &q)
{
    return q.display(os);
}
//binaryquery 类
class BinaryQuery:public Query_base
{
protected:
    BinaryQuery(Query left,Query right,std::string op):lhs(left),rhs(right),oper(op)
    {
    }
    //输出两个Query对象,以及运算符
    std::ostream& display(std::ostream &os) const
    {
        return os<<"("<<lhs<<" "<<oper<<" "<<rhs<<")";
    }
    const Query lhs,rhs;
    const std::string oper;
};

class AndQuery:public BinaryQuery
{
    friend Query operator &(const Query&,const Query&);
    AndQuery(Query left,Query right):BinaryQuery(left,right,"&")
    {
    }
    std::set<line_no> eval(const TextQuery&) const;
};

class OrQuery:public BinaryQuery
{
    friend Query operator|(const Query&,const Query&);
    OrQuery(Query left,Query right):BinaryQuery(left,right,"|")
    {
    }
    std::set<line_no> eval(const TextQuery&) const;
};

class WordQuery:public Query_base
{
    friend class Query;
    WordQuery(const std::string &s):query_word(s)
    {
    }
    //执行运算
    std::set<line_no> eval(const TextQuery &t) const
    {
        return t.run_query(query_word);
    }
    //输出查询的单词
    std::ostream& display(std::ostream &os) const
    {
        return os<<query_word;
    }
    std::string query_word;
};

inline Query::Query(const std::string &s): q(new WordQuery(s)),
                                           use(new std::size_t(1)) { }


class NotQuery:public Query_base
{
    friend Query operator ~(const Query &);
    NotQuery(Query q):query(q){}
    std::set<line_no> eval (const TextQuery&) const;
    std::ostream& display(std::ostream &os) const
    {
        return os<<"~("<<query<<")";
    }
    const Query query;
};
//重载运算符 &
inline Query operator&(const Query &lhs, const Query &rhs)
{
    return new AndQuery(lhs,rhs);
}
//重载运算符 |
inline Query operator|(const Query &lhs,const Query &rhs)
{
    return new OrQuery(lhs,rhs);
}
//重载运算符 ~
inline Query operator~(const Query &oper)
{
    return new NotQuery(oper);
}


//取并集
set<TextQuery::line_no> OrQuery::eval(const TextQuery&file)const
{
    set<line_no> right=rhs.eval(file),ret_lines=lhs.eval(file);
    ret_lines.insert(right.begin(),right.end());
    return ret_lines;
}
//取交集
set<TextQuery::line_no> AndQuery::eval(const TextQuery& file)const
{
    set<line_no> left=lhs.eval(file),right=rhs.eval(file);
    set<line_no> ret_lines;

    set_intersection(left.begin(),left.end(),right.begin(),right.end(),inserter(ret_lines,ret_lines.begin()));
    return ret_lines;
}
//取差集
set<TextQuery::line_no> NotQuery::eval(const TextQuery& file)const
{
    set<TextQuery::line_no> has_val=query.eval(file);
    set<line_no> ret_lines;

    for(TextQuery::line_no n=0;n!=file.size();++n)
        if(has_val.find(n)==has_val.end())
            ret_lines.insert(n);
    return ret_lines;
}

TextQuery build_textfile(const string &filename)
{
    // get a file to read from which user will query words
    ifstream infile;
    if (!open_file(infile, filename)) {
        cerr << "No input file!" << endl;
        return TextQuery();
    }

    TextQuery ret;
    ret.read_file(infile);  // builds query map
    return ret;  // builds query map
}
bool get_word(string &s1)
{
    cout << "enter a word to search for, or q to quit: ";
    cin >> s1;
    if (!cin || s1 == "q") return false;
    else return true;
}
bool get_words(string &s1, string &s2)
{

    // iterate with the user: prompt for a word to find and print results
    cout << "enter two words to search for, or q to quit: ";
    cin  >> s1;

    // stop if hit eof on input or a "q" is entered
    if (!cin || s1 == "q") return false;
    cin >> s2;
    return true;
}

int main(int, char **argv)
{
    // gets file to read and builds map to support queries
    TextQuery file = build_textfile("/Users/zhouyang/Desktop/text.txt");

    // iterate with the user: prompt for a word to find and print results
    while (true) {
        string sought1, sought2, sought3;
        if (!get_words(sought1, sought2)) break;
        cout << "\nenter third word: " ;
        cin  >> sought3;
        // find all the occurrences of the requested string
        Query q = Query(sought1) & Query(sought2)
                             | Query(sought3);

        cout << "\nExecuting Query for: " << q << endl;
        const set<TextQuery::line_no> locs = q.eval(file);
        // report matches
        print_results(locs, file);
     }
     return 0;
}

