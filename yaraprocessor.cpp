#include "yaraprocessor.h"

#include <QTextStream>
#include <QDebug>

YaraProcessor::YaraProcessor(QObject *parent)
    : QObject{parent},
    m_compiler(NULL),
    m_rules(NULL)
{

}


YaraProcessor::~YaraProcessor()
{
    for(auto rule_set : m_rule_sets)
    {
        yr_rules_destroy(rule_set);
    }
}

void YaraProcessor::clearRules()
{
    for(auto rule_set : m_rule_sets)
    {
        yr_rules_destroy(rule_set);
    }

    m_rule_sets.clear();
}

void YaraProcessor::compilerCallback(int error_level, const char *file_name, int line_number, const YR_RULE *rule, const char *message, void *user_data)
{
    YaraProcessor *instance = static_cast<YaraProcessor*>(user_data);

    QStringList file_path = QString(file_name).split("/YARA/");

    if(error_level == YARA_ERROR_LEVEL_ERROR)
    {
        QString error_str = "<font color='#FF6961'>[ ! ] Compilation error: </font>"
                            + QString(message)
                            + " in file "
                            + "<font color='#76a4bd'>" + file_path.at(1) + "</font>"
                            + " at line "
                            + QString::number(line_number);

        emit instance->yaraError(error_str);
    }
    else if(error_level == YARA_ERROR_LEVEL_WARNING)
    {
        QString warning_str = "<font color='#FFC879'>[ * ] Warning: </font>"
                            + QString(message)
                            + " in file "
                            + "<font color='#76a4bd'>" + file_path.at(1) + "</font>"
                            + " at line "
                            + QString::number(line_number);

        emit instance->yaraWarning(warning_str);
    }
}


void YaraProcessor::compileYaraRules(const QStringList &rule_file_path_list)
{
    if(yr_compiler_create(&m_compiler) != ERROR_SUCCESS)
    {
        emit yaraError("<font color='#FF6961'>[ ! ] Unable to create YARA compiler.</font>");
        return;
    }

    yr_compiler_set_callback(m_compiler, compilerCallback, this);

    for(const QString &rule_file_path : rule_file_path_list)
    {
        QStringList file_path = rule_file_path.split("/YARA/");

        FILE *rule_file = fopen(rule_file_path.toStdString().c_str(), "r");
        if(!rule_file)
        {
            emit yaraWarning("<font color='#FFC879'>[ * ] Unable to open rules: </font>" + file_path.at(1));
            continue;
        }

        if(yr_compiler_add_file(m_compiler, rule_file, NULL, rule_file_path.toStdString().c_str()) > 0)
        {
            fclose(rule_file);
            continue;
        }

        emit yaraSuccess("<font color='#81bd76'>[ + ] Compiled rules: </font>" + file_path.at(1));
        fclose(rule_file);
    }

    if(yr_compiler_get_rules(m_compiler, &m_rules) != ERROR_SUCCESS)
    {
        emit yaraError("<font color='#FF6961'>[ ! ] Unable to get YARA rules.</font>");
        yr_compiler_destroy(m_compiler);
        return;
    }

    m_rule_sets.push_back(m_rules);
    yr_compiler_destroy(m_compiler);
    m_rules = NULL;
}


void YaraProcessor::loadCompiledYaraRules(const QStringList &compiled_rule_path_list)
{
    for(const QString &compiled_rule_path : compiled_rule_path_list)
    {
        QStringList file_path = compiled_rule_path.split("/YARA/");

        YR_RULES *compiled_rules;
        if(yr_rules_load(compiled_rule_path.toStdString().c_str(), &compiled_rules) != ERROR_SUCCESS)
        {
            emit yaraWarning("<font color='#FFC879'>[ * ] Unable to load compiled rules: </font>" + file_path.at(1));
            continue;
        }
        else
            emit yaraSuccess("<font color='#81bd76'>[ + ] Loaded compiled rules: </font>" + file_path.at(1));

        m_rule_sets.push_back(compiled_rules);
    }
}


int YaraProcessor::yaraCallback(YR_SCAN_CONTEXT*, int message, void *message_data, void *user_data)
{
    if(message == CALLBACK_MSG_RULE_MATCHING)
    {
        YR_RULE *rule = (YR_RULE*)message_data;

        auto user_data_pair = static_cast<std::pair<const char*, YaraProcessor*>*>(user_data);
//        emit user_data_pair->second->matchedRule("Matched rule: <font color='#81bd76'>" + QString(rule->identifier) + "</font> in file: " + user_data_pair->first);
        user_data_pair->second->match.append(QString(rule->identifier) + " | ");
    }

    return CALLBACK_CONTINUE;
}


QString YaraProcessor::scanFile(const QString &file_path)
{
    match = "";

    for(auto rule_set : m_rule_sets)
    {
        QByteArray file_bytes = file_path.toLocal8Bit();
        char *char_file_path = file_bytes.data();

        auto user_data = new std::pair<const char*, YaraProcessor*>(char_file_path, this);

        int result = yr_rules_scan_file(rule_set, char_file_path, 0, yaraCallback, user_data, 0);

        if(result != ERROR_SUCCESS)
        {
            emit yaraError("<font color='red'>[!] Error scanning file: </font>" + file_path);
        }

        delete user_data;
    }

    match.chop(3);
    return match;
}

