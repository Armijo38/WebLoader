#ifndef NETWORKREQUEST_H
#define NETWORKREQUEST_H

#include <QEventLoop>
#include <QTimer>

#include "NetworkQueue.h"
#include "NetworkRequestInternal.h"

/*!
 * \brief Пользовательский класс для создания GET и POST запросов
 */

class NetworkRequest : public QObject
{
    Q_OBJECT
public:

    /*!
     * \brief Конструктор
     */
    explicit NetworkRequest(QObject * _parent = 0, QNetworkCookieJar * _jar = 0);
    /*!
     * \brief Деструктор
     */
    virtual ~NetworkRequest();

    /*!
     * \brief Установка cookie для загрузчика
     */
    void setCookieJar(QNetworkCookieJar *cookieJar);
    /*!
     * \brief Получение cookie загрузчика
     */
    QNetworkCookieJar* getCookieJar();
    /*!
     * \brief Установка метода запроса
     */
    void setRequestMethod(WebLoader::RequestMethod method);
    /*!
     * \brief Получение метода запроса
     */
    WebLoader::RequestMethod getRequestMethod() const;
    /*!
     * \brief Установка таймаута загрузки
     */
    void setLoadingTimeout(int loadingTimeout);
    /*!
     * \brief Получение таймаута загрузки
     */
    int getLoadingTimeout() const;
    /*!
     * \brief Очистить все старые атрибуты запроса
     */
    void clearRequestAttributes();
    /*!
     * \brief Добавление атрибута в запрос
     */
    void addRequestAttribute(QString name, QVariant value);
    /*!
     * \brief Добавление файла в запрос
     */
    void addRequestAttributeFile(QString name, QString filePath);
    /*!
     * \brief Асинхронная загрузка запроса
     */
    void loadAsync(QString urlToLoad, QUrl referer = QUrl());
    /*!
     * \brief Асинхронная загрузка запроса
     */
    void loadAsync(QUrl urlToLoad, QUrl referer = QUrl());
    /*!
     * \brief Синхронная загрузка запроса
     */
    QByteArray loadSync(QString urlToLoad, QUrl referer = QUrl());
    /*!
     * \brief Синхронная загрузка запроса
     */
    QByteArray loadSync(QUrl urlToLoad, QUrl referer = QUrl());
    /*!
     * \brief Получение загруженного URL
     */
    QUrl url() const;
    /*!
     * \brief Получение строки с последней ошибкой
     */
    QString lastError() const;
    QString lastErrorDetails() const;

signals:
    /*!
     * \brief Прогресс отправки запроса на сервер
     */
    void uploadProgress(int);
    /*!
     * \brief Прогресс загрузки данных с сервера
     */
    void downloadProgress(int);
    /*!
     * \brief Данные загружены
     */
    void downloadComplete(QByteArray);
    void downloadComplete(QString);
    /*!
     * \brief Сигнал об ошибке
     */
    void error(QString);
    void finished();

private:

    /*!
     * \brief Объект, используемый в очереди запросов
     */
    NetworkRequestInternal internal;
    /*!
     * \brief Загруженные данные в случае, если используется синхронная загрузка
     */
    QByteArray m_downloadedData;
    /*!
     * \brief Строка, содержащая описание последней ошибки
     */
    QString m_lastError;

    /*!
     * \brief Остановка выполнения запроса, связанного с текущим объектом
     * и удаление запросов, ожидающих в очереди, связанных с текущим объектом
     */
    void stop();

private slots:
    /*!
     * \brief Данные загружены. Используется при синхронной загрузке
     */
    void downloadCompleteData(QByteArray);
    /*!
     * \brief Ошибка при получении данных
     */
    void slotError(QString);
};

#endif // NETWORKREQUEST_H
