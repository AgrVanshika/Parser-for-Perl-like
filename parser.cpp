/* Implementation of Recursive-Descent Parser
 * parserInt.cpp
 * Programming Assignment 3
 * Spring 2023
*/

#include "parserInt.h"
#include "val.h"

map<string, bool> defVar;
map<string, Token> SymTable;
map<string, Value> TempsResults; //Container of temporary locations of Value objects for results of expressions, variables values and constants 
queue <Value> * ValQue; //declare a pointer variable to a queue of Value objects

namespace Parser {
    bool pushed_back = false;
    LexItem	pushed_token;

    static LexItem GetNextToken(istream& in, int& line) {
        if( pushed_back ) {
            pushed_back = false;
            return pushed_token;
        }
        return getNextToken(in, line);
    }

    static void PushBackToken(LexItem & t) {
        if( pushed_back ) {
            abort();
        }
        pushed_back = true;
        pushed_token = t;
    }

}

static int error_count = 0;

int ErrCount()
{
    return error_count;
}

void ParseError(int line, string msg)
{
    ++error_count;
    cout << error_count << ". Line # " << line << ": " << msg << endl;
}
void pushBackTokenWrapper(LexItem& token) {
    Parser::PushBackToken(token);
}
bool Prog(istream& in, int& line) {
    if (!StmtList(in, line)) {
        ParseError(line, "Error in Program");
        return 0;
    }
    cout << "\n(DONE)" << endl;
    return 1;
}

// StmtList ::= Stmt ;{ Stmt; 
bool StmtList(istream& in, int& line){
    bool status;

    LexItem kot;

    status = Stmt(in, line);
    while(status){
        kot = Parser::GetNextToken(in, line);

        if(kot == DONE){        
        Parser::PushBackToken(kot);
            if (in.eof()){ // Check if the end of the file is reached             
                return 1;
            }
        }
        else if(kot == RBRACES){
            Parser::PushBackToken(kot);
            return 1;
        }

        if(kot != SEMICOL){
            ParseError(line, "Missing semicolon at end of Statement.");
            return 0;
        }
        status = Stmt(in, line);

    }

    kot = Parser::GetNextToken(in, line);
    if(kot == ELSE){
        ParseError(line, "Missing right brace.");
        return 0;
    }
    else if(kot == RBRACES){
        Parser::PushBackToken(kot);
        return 0;
    }

    else{
        ParseError(line, "Syntactic error in Program Body.");
        return 0;
    }
} 

bool Stmt(istream& in, int& line){
    bool stmt=1;
    LexItem t;
    t = Parser::GetNextToken(in, line);

    switch( t.GetToken() ) {
        case SIDENT: case NIDENT:
            Parser::PushBackToken(t);            
            stmt = AssignStmt(in, line);
            if(!stmt){
                ParseError(line, "Incorrect Assignment Statement.");
                return 0;
            }
            break;
        case WRITELN:
            stmt = WritelnStmt(in, line);
            if(!stmt){
                ParseError(line, "Incorrect Writeln Statement.");
                return 0;
            }
            break;
        case IF:
            stmt = IfStmt(in, line);
            if(!stmt){
                ParseError(line, "Incorrect If-Statement.");
                return 0;
            }            
            break;
        case ELSE:
            Parser::PushBackToken(t);            
            return 1;
        case IDENT:
            ParseError(line, "Invalid variable name");
            Parser::PushBackToken(t);            
            return 1;
        default:
            Parser::PushBackToken(t);           
            return 1;
    }
    return stmt;
}

bool IfStmt(istream& in, int& line) {
    Value retVal;

    LexItem t = Parser::GetNextToken(in, line);
    if (t != LPAREN) {
        ParseError(line, "Missing left parenthesis in if statement");
        return 0;
    }
    bool ex_status = Expr(in, line, retVal);
    if (!ex_status) {
        ParseError(line, "Invalid expression in if statement");
        return 0;
    }

    if (retVal.GetType() != VBOOL) {
        ParseError(line, "Illegal Type for If statement condition.");
        return 0;
    }
   
    t = Parser::GetNextToken(in, line);
    if (t != RPAREN) {
        ParseError(line, "Missing right parenthesis for If statement condition");
        return 0;
    }

    t = Parser::GetNextToken(in, line);
    if (t != LBRACES) {
        ParseError(line, "Missing left brace in if statement");
        return 0;
    }

    if (retVal.GetBool()) {
        bool stmtlist_status = StmtList(in, line);
        if (!stmtlist_status) {
            ParseError(line, "Missing Statement for If-Stmt Clause");
            return 0;
        }

        t = Parser::GetNextToken(in, line);
        if (t != RBRACES) {
            ParseError(line, "Missing right brace in if statement");
            return 0;
        }

        t = Parser::GetNextToken(in, line);
        if (t != ELSE) {
            // No "else" clause, so we're done
            Parser::PushBackToken(t);
            return 1;
        }
    
        t = Parser::GetNextToken(in, line);
        if (t != LBRACES) {
            ParseError(line, "Missing left brace in else clause of if statement");
            return 0;
        }

        t = Parser::GetNextToken(in, line);
        while (t != RBRACES) {
            if (t == DONE) {
                ParseError(line, "Missing right brace in else clause of if statement");
                return 0;
            }
           t = Parser::GetNextToken(in, line);
        }
    }
    else {
        t = Parser::GetNextToken(in, line);
        while (t != RBRACES) {
            if (t == DONE || t == ELSE) {
                ParseError(line, "Missing right brace in if clause of if statement");
                return 0;
            }
            t = Parser::GetNextToken(in, line);
        }
    
        t = Parser::GetNextToken(in, line);
        if (t != ELSE) {
            // No "else" clause, so we're done
            Parser::PushBackToken(t);
            return 1;
        }
    
        t = Parser::GetNextToken(in, line);
        if (t != LBRACES) {
            ParseError(line, "Missing left brace in else clause of if statement");
            return 0;
        }

        bool stmtlist_status = StmtList(in, line);
        if (!stmtlist_status) {
            ParseError(line, "Invalid statement list in if statement");
            return 0;
        }

        t = Parser::GetNextToken(in, line);
        if (t != RBRACES) {
            ParseError(line, "Missing right brace in if statement");
            return 0;
        }
    }
    return 1;
}
//WritelnStmt:= WRITELN (ExpreList)
bool WritelnStmt(istream& in, int& line) {
    LexItem t;
    ValQue = new queue<Value>;

    t = Parser::GetNextToken(in, line);
    if( t != LPAREN ) {

        ParseError(line, "Missing Left Parenthesis of Writeln Statement");
        return 0;
    }

    bool ex = ExprList(in, line);

    if( !ex ) {
        ParseError(line, "Missing expression list after Print");
        while(!(*ValQue).empty())
        {
            ValQue->pop();
        }
        delete ValQue;
        return 0;
    }


    //Evaluate: writeln by printing out the list of expressions' values
    while (!(*ValQue).empty())
    {
        Value nextVal = (*ValQue).front();
        cout << nextVal;
        ValQue->pop();
    }
    cout << endl;

    t = Parser::GetNextToken(in, line);
    if(t != RPAREN ) {

        ParseError(line, "Missing Right Parenthesis of Writeln Statement");
        return 0;
    }
    //Parser::PushBackToken(t);
    return 1;
}//End of WritelnStmt

bool AssignStmt(istream& in, int& line) {
    LexItem kot, var = Parser::GetNextToken(in, line);
    string lexeme = var.GetLexeme();
    Value retVal;
    kot = Parser::GetNextToken(in, line);
    if (kot != ASSOP) {
        if (kot == SEMICOL) {
            Parser::PushBackToken(kot);
            defVar[lexeme] = 0;
            return 1;
        }
        ParseError(line, "Missing assign operator after variable");
        return 0;
    }

    if (!Expr(in, line, retVal)) {
        ParseError(line, "Missing Expression in Assignment Statement");
        return 0;
    }
    if (var == NIDENT and !(retVal.IsInt() or retVal.IsReal())) {
        ParseError(line, "Invalid assignment statement conversion of a string value to a double variable.");
        return 0;
    }

    if (var == SIDENT and !(retVal.IsInt() or retVal.IsReal() or retVal.IsString())) {
        ParseError(line, "Missing Expression in Assignment Statement");
        return 0;
    }

    defVar[lexeme] = 1;
    TempsResults[lexeme] = retVal;
    return 1;
}
bool Var(istream& in, int& line, LexItem& idtok)
{
    LexItem kot = Parser::GetNextToken(in, line);
    if (kot == NIDENT || kot == SIDENT){
        idtok = kot; // Set idtok to the identifier token
        string identstr = kot.GetLexeme();
        if (!(defVar.find(identstr)->second))
        {
            defVar[identstr] = 1;
            SymTable[identstr] = kot.GetToken();
        }
        return 1;
    }
    else if(kot.GetToken() == ERR){
        ParseError(line, "Unrecognized Input Pattern");
        return 0;
    }
    return 1;
}

//ExprList:= Expr {,Expr}
bool ExprList(istream& in, int& line) {
    bool status = 0;
    Value retVal;

    status = Expr(in, line, retVal);
    if(!status){
        ParseError(line, "Missing Expression");
        return 0;
    }
    ValQue->push(retVal);
    LexItem kot = Parser::GetNextToken(in, line);

    if (kot == COMMA) {
        status = ExprList(in, line);
    }
    else if(kot.GetToken() == ERR){
        ParseError(line, "Unrecognized Input Pattern");
        cout << "(" << kot.GetLexeme() << ")" << endl;
        return 0;
    }
    else{
        Parser::PushBackToken(kot);
        return 1;
    }
    return status;
}//End of ExprList
bool Expr(istream& in, int& line, Value& retVal) {
    if (!RelExpr(in, line, retVal)) {
        return 0;
    }
    LexItem kot = Parser::GetNextToken(in, line);
    if (kot == SEQ || kot == NEQ) {
        Value nextVal;
        if (!RelExpr(in, line, nextVal)) {
            ParseError(line, "Missing opperand after operator");
            return 0;
        }
        if (kot == SEQ){
            retVal = retVal.SEqual(nextVal);
        }
        else{
            retVal = retVal == nextVal;
        }
        if (retVal.GetType() == VERR) {
            ParseError(line, "Illegal operand type for the operation.");
            return 0;
        }
    }
    else{ 
        
        Parser::PushBackToken(kot);
        return 1;
        }

    return 1;
}

bool RelExpr(istream& in, int& line, Value& retVal) {
    bool stats = AddExpr(in, line, retVal);
    LexItem tokie = Parser::GetNextToken(in, line);
    if (tokie == NLTHAN || tokie == NGTHAN || tokie == SLTHAN || tokie == SGTHAN) {
        Value nextVal;
        if (!AddExpr(in, line, nextVal)) {
            return 0;
        }
        if (tokie == NLTHAN) {
            retVal = retVal < nextVal;
        } else if (tokie == NGTHAN) {
            retVal = retVal > nextVal;
        } else if (tokie == SLTHAN) {
            retVal = retVal.SLthan(nextVal);
        } else if (tokie == SGTHAN) {
            retVal = retVal.SGthan(nextVal);
        }
        stats = 1;
        tokie = Parser::GetNextToken(in, line);
    }
    Parser::PushBackToken(tokie);
    return stats;
}

bool AddExpr(istream& in, int& line, Value& retVal) {
    if (!MultExpr(in, line, retVal)) {
        return 0;
    }
    LexItem kot = Parser::GetNextToken(in, line);
    while (kot == PLUS || kot == MINUS || kot == CAT) {
        Value tempVal;
        if (!MultExpr(in, line, tempVal)) {
            ParseError(line, "Missing operand after operator");
            return 0;
        }
        if (kot == PLUS) {
            retVal =  retVal + tempVal;
            if (retVal.GetType()== VERR)
            {
            ParseError(line, "Illegal add operation.");
                return 0;
            }
        } else if (kot == MINUS) {
            retVal = retVal - tempVal;
            if (retVal.GetType()== VERR)
            {
                ParseError(line, "Illegal sub operation.");
                return 0;
            }
        } else if (kot == CAT) {
            retVal = retVal.Catenate(tempVal);
            if (retVal.GetType()== VERR)
            {
                ParseError(line, "Illegal catenate operation.");
                return 0;
            }

        }
        kot = Parser::GetNextToken(in, line);
        if(kot.GetToken() == ERR){
            ParseError(line, "Unrecognized Input Pattern");
            cout << "(" << kot.GetLexeme() << ")" << endl;
            return 0;
        }
    }
    Parser::PushBackToken(kot);
    return 1;
}

bool MultExpr(istream& in, int& line, Value& retVal) {
    //bool status = ExponExpr(in, line, retVal);
    if (!ExponExpr(in, line, retVal)) return 0;

    LexItem kot = Parser::GetNextToken(in, line);
    while (kot == MULT || kot == DIV || kot == SREPEAT) {
        Value val_this = retVal;
        if (!ExponExpr(in, line, retVal)) {
            ParseError(line, "Missing opperand after operator");
            return 0;
        }
        if (kot == MULT) retVal = val_this * retVal;
        else if (kot == DIV) retVal = val_this / retVal;
        else retVal = val_this.Repeat(retVal);
        
        kot = Parser::GetNextToken(in, line);

        if (retVal.GetType() == VERR) {
            ParseError(line, "Illegal operand type for the operation.");
            return 0;
        }
    }
    Parser::PushBackToken(kot);
    return 1;
}

bool ExponExpr(istream& in, int& line, Value& retVal) {
    bool status;

    status = UnaryExpr(in, line,retVal);
    if( !status ) {
        return 0;
    }
    LexItem kot = Parser::GetNextToken(in, line);
    while (kot == EXPONENT) {
        Value next_Val;
        if (!ExponExpr(in, line, next_Val)) {
            ParseError(line, "Missing operand after operator");
            return 0;
        }
        retVal = retVal ^ next_Val;
        kot = Parser::GetNextToken(in, line);
    }

    Parser::PushBackToken(kot);
    return 1;
}


//UnaryExpr ::= [( - | + )] PrimaryExpr
bool UnaryExpr(istream& in, int& line, Value& retVal) {
    bool status = 0;
    LexItem t = Parser::GetNextToken(in, line);
    if (t == MINUS || t == PLUS) {
        status = PrimaryExpr(in, line, t == MINUS ? -1 : 1, retVal);
    } else {
        pushBackTokenWrapper(t);
        status = PrimaryExpr(in, line, 1, retVal);
    }
    return status;
}
bool PrimaryExpr(istream& in, int& line, int sign, Value& retVal) {
    LexItem kot = Parser::GetNextToken(in, line);
    string lexeme = kot.GetLexeme();
    if (kot == NIDENT || kot == SIDENT) {
    
        if (!(defVar.find(lexeme)->second)) {
            ParseError(line, "Using Undefined Variable");
            return 0;
        }
        retVal = TempsResults[lexeme];
        // Apply the sign to the value
        if (retVal.IsInt()) {
            retVal.SetInt(sign * retVal.GetInt());
        } else if (retVal.IsReal()) {
            retVal.SetReal(sign * retVal.GetReal());
        } else if (sign == -1) {
            ParseError(line, "Illegal Operand Type for Sign Operator");
            return 0;
        }
    }
    else if (kot == ICONST || kot == RCONST) {
        if(lexeme[lexeme.size() - 1] == '.'){    
         //cout<<"WTF IS THIS GIVING eror "<<endl;
            int val = stoi(kot.GetLexeme());
            retVal.SetType(VINT);
            retVal.SetInt(val);
            return 1;
        }else{
            double val = stod(kot.GetLexeme());
            retVal.SetType(VREAL);
            retVal.SetReal(val);
            return 1;
        }
        return 1;
    }
    else if (kot == SCONST) {
        if (sign == -1) {
            ParseError(line, "Illegal Operand Type for Sign Operator");
            return 0;
        }
        retVal.SetType(VSTRING);
        retVal.SetString(kot.GetLexeme());
    }
    else if (kot == RCONST) {
        double val = stod(kot.GetLexeme());
        retVal.SetType(VREAL);
        retVal.SetReal(sign * val);
    }
    else if (kot == LPAREN) {
        if (sign == -1) {
            ParseError(line, "Invalid use of '-' with a parenthetical expression");
            return 0;
        }
        bool ex = Expr(in, line, retVal);
        if (!ex) {
            ParseError(line, "Missing expression after Left Parenthesis");
            return 0;
        }
        if (Parser::GetNextToken(in, line) == RPAREN) {
            return ex;
        }

    }
    else if (kot.GetToken() == ERR) {
        ParseError(line, "Unrecognized Input Pattern");
        cout << "(" << kot.GetLexeme() << ")" << endl;
        return 0;
    }

    return 1;
}