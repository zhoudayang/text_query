#include <iostream>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

using namespace std;

class TextQuery {
public:
    //执行查找,返回单词所在行号组成的set
    set<int> run_query(const string &word) const {
        auto it = word_map.find(word);
        if (it == word_map.end())
            return set<int>();
        return it->second;
    }

    //返回line行对应的字符串
    string text_line(int line) const {
        if (line < lines_of_text.size())
            return lines_of_text[line];
        throw out_of_range("line number out of range");
    }

    //返回vector中存储的行的数目
    int size() const {
        return lines_of_text.size();
    }

    //读取文件,建立键值对映射关系
    void read_file(ifstream &is) {
        store_file(is);
        build_map();
    }

private:
    //从文件输入流in中读取行,并将其存储在vector中
    void store_file(ifstream &is) {
        string textline;
        while (getline(is, textline))
            lines_of_text.push_back(textline);
    }

    //建立单词 和行号组成的键值对映射关系
    void build_map() {
        for (int line_no = 0; line_no != lines_of_text.size(); line_no++) {
            string text_line = lines_of_text[line_no];
            istringstream line(text_line);
            string word;
            while (line >> word) {
                word_map[word].insert(line_no);
            }

        }
    }

    //存储所有的行
    vector<string> lines_of_text;
    //存储单词,行号键值对
    map<string, set<int>> word_map;
};

//显示查询结果
void print_results(const set<int> &locs, const TextQuery &file) {
    if (locs.empty()) {
        cout << endl << "Sorry. There are no entries for your query." << endl << "Try again." << endl;
        return;
    }
    //输出行号和对应的行
    for (auto it = locs.begin(); it != locs.end(); it++) {
        cout << "\t" << "(line "
        << (*it) + 1 << ") "
        << file.text_line(*it) << endl;
    }
}

//打开文件,返回文件输入流
ifstream &open_file(ifstream &in, const string &file) {
    in.close();
    in.clear();
    in.open(file.c_str());
    return in;
}

class Query_base {
    friend class Query;

protected:
    virtual ~Query_base() { }

private:
    //定义纯虚函数,对TextQuery对象执行查询
    virtual set<int> eval(const TextQuery &) const = 0;
    //输出查询结果
    virtual ostream &display(ostream & = cout) const = 0;
};
//对单词执行查询
class WordQuery : public Query_base {
    friend class Query;

    WordQuery(const string &s) : query_word(s) { }

    //执行查询,返回查询的结果
    set<int> eval(const TextQuery &t) const {
        return t.run_query(query_word);
    }
    //输出查询结果
    ostream &display(ostream &os) const {
        return os << query_word;
    }
    //要查询的单词
    string query_word;
};

class Query {
    //这些函数能够隐式调用private中的构造函数 Query(Query_base *query),创建Query对象
    friend Query operator~(const Query &);

    friend Query operator|(const Query &, const Query &);

    friend Query operator&(const Query &, const Query &);

public:
    //对要查询的单词初始化Query对象
    Query(const string &s) : q(new WordQuery(s)), use(new int(1)) {

    }
    //复制构造函数
    Query(const Query &c) : q(c.q), use(c.use) {
        ++*use;
    }
    //析构函数
    ~Query() {
        decr_use();
    }
    //重载运算符=
    Query &operator=(const Query &);
    //对TextQuery 执行查询任务
    set<int> eval(const TextQuery &t) const {
        return q->eval(t);
    }

    ostream &display(ostream &os) const {
        return q->display(os);
    }

private:
    //友元重载函数可以访问
    Query(Query_base *query) : q(query), use(new int(1)) { }

    Query_base *q;
    int *use;
    //减少引用计数的值,如果引用计数的值为0,那么删除对象
    void decr_use() {
        if (--*use == 0) {
            delete q;
            delete use;
        }
    }
};
//重载运算符 = ,减少本对象的引用计数,同时增加要复制的对象的引用计数
Query &Query::operator=(const Query &rhs) {
    ++*rhs.use;
    decr_use();
    q = rhs.q;
    use = rhs.use;
    return *this;
}
//重载运算符 <<
ostream &operator<<(ostream &os, const Query &q) {
    return q.display(os);
}

class BinaryQuery : public Query_base {
protected:
    BinaryQuery(Query left, Query right, string op) : lhs(left), rhs(right), oper(op) { }
    //输出 查询 操作
    ostream &display(ostream &os) const {
        return os << "(" << lhs << " " << oper << " " << rhs << ")";
    }
    //左右两个操作数
    const Query lhs, rhs;
    //运算符
    const string oper;
};

class AndQuery : public BinaryQuery {
    //友元重载运算符函数可以访问构造器函数 Query(Query_base *query)
    friend Query operator&(const Query &, const Query &);

    AndQuery(Query left, Query right) : BinaryQuery(left, right, "&") { }
    //查询实际上是对左右操作数的查询结果取交集
    set<int> eval(const TextQuery &file) const {
        set<int> left = lhs.eval(file);
        set<int> right = rhs.eval(file);
        set<int> ret;
        set_intersection(left.begin(), left.end(), right.begin(), right.end(), inserter(ret, ret.begin()));
        return ret;
    }
};

class OrQuery : public BinaryQuery {
    //友元重载运算符函数可以访问构造器函数Query(Query_base *query)
    friend Query operator|(const Query &, const Query &);

    OrQuery(Query left, Query right) : BinaryQuery(left, right, "|") { }
    //查询实际上是对左右操作数的查询结果取并集
    set<int> eval(const TextQuery &file) const {
        set<int> left = lhs.eval(file);
        set<int> right = rhs.eval(file);
        left.insert(right.begin(), right.end());
        return left;
    }
};

class NotQuery : public Query_base {
    //友元重载运算符函数可以访问构造器函数 Query(Query_base *query)
    friend Query operator~(const Query &);

    NotQuery(Query q) : query(q) { };
    //执行的查询实际上是对左右两个操作数的查询结果取差集
    set<int> eval(const TextQuery &file) const {
        auto result = query.eval(file);
        set<int> ret;
        for (int n = 0; n != file.size(); n++) {
            if (result.find(n) == result.end())
                ret.insert(n);
        }
        return ret;
    }

    ostream &display(ostream &os) const {
        return os << "~" << query << ")";
    }

    const Query query;
};

//重载运算符 &
inline Query operator&(const Query &lhs, const Query &rhs) {
    return new AndQuery(lhs, rhs);
}

//重载运算符 |
inline Query operator|(const Query &lhs, const Query &rhs) {
    return new OrQuery(lhs, rhs);
}

//重载运算符 ~
inline Query operator~(const Query &oper) {
    return new NotQuery(oper);
}

//创建TextQuery实例
TextQuery build_textfile(const string &filename) {
    ifstream infile;
    if (!open_file(infile, filename)) {
        cerr << "can't open input file!" << endl;
        return TextQuery();
    }
    TextQuery ret;
    ret.read_file(infile);
    return ret;
}

int main() {
    TextQuery file = build_textfile("/Users/zhouyang/Desktop/text.txt");
    string word1, word2, word3;
    while (cin >> word1 >> word2 >> word3) {
        Query q = Query(word1) & Query(word2) | Query(word3);
        cout << "Executing Query for: " << q << endl;
        auto result = q.eval(file);
        print_results(result, file);
    }
    return 0;
}
