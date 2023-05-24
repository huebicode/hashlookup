#ifndef YARAPROCESSOR_H
#define YARAPROCESSOR_H

#include <QObject>

#include "yara.h"

class YaraProcessor : public QObject
{
    Q_OBJECT
public:
    explicit YaraProcessor(QObject *parent = nullptr);

    ~YaraProcessor();

    void compileYaraRules(const QStringList &rule_file_path_list);

    void loadCompiledYaraRules(const QStringList &compiled_rule_path_list);

    QString scanFile(const QString &file_path);

    void clearRules();

signals:
    void yaraWarning(QString warning);
    void yaraError(QString error);
    void yaraSuccess(QString success);
    void matchedRule(QString matched_rule);

private:
    static void compilerCallback(int error_level, const char *file_name, int line_number, const YR_RULE *rule, const char* message, void *user_data);

    static int yaraCallback(YR_SCAN_CONTEXT *context, int message, void *message_data, void *user_data);

    QString match;

    YR_COMPILER *m_compiler;
    YR_RULES *m_rules;
    QVector<YR_RULES*> m_rule_sets;

    const QStringList m_rule_file_paths;
    const QStringList m_compiled_rule_paths;
};

#endif // YARAPROCESSOR_H
