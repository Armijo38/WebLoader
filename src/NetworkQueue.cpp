#include "NetworkQueue.h"

NetworkQueue::NetworkQueue()
{
    //
    // В нужном количестве создадим WebLoader'ы
    // И сразу же соединим их со слотом данного класса, обозначающим завершение
    //
    for (int i = 0; i != qMax(QThread::idealThreadCount(), 4); ++i) {
        m_freeLoaders.push_back(new WebLoader(this));
        connect(m_freeLoaders.back(), SIGNAL(downloadComplete(WebLoader*)), this, SLOT(downloadComplete(WebLoader*)));
    }
}

NetworkQueue* NetworkQueue::getInstance() {
    static NetworkQueue queue;
    return &queue;
}

void NetworkQueue::put(NetworkRequestInternal* _request) {
    m_mtx.lock();
    //
    // Положим в очередь пришедший запрос
    //
    m_queue.push_back(_request);
    m_inQueue.insert(_request);

    //
    // В случае, если есть свободный WebLoader
    // Извлечем пришедший запрос из очереди и начнем выполнять его
    //
    if (!m_freeLoaders.empty()) {
        pop();
    }
    m_mtx.unlock();
}

void NetworkQueue::pop() {
    //
    // Извлечем свободный WebLoader
    //
    WebLoader* loader = m_freeLoaders.front();
    m_freeLoaders.pop_front();

    //
    // Извлечем первый запрос на обработку
    //
    NetworkRequestInternal *request = m_queue.front();
    m_queue.pop_front();
    m_inQueue.remove(request);

    //
    // Настроим WebLoader на запрос
    //
    m_busyLoaders[loader] = request;
    setLoaderParams(loader, request);

    //
    // Соединим сигналы WebLoader'а с сигналами класса запроса
    //
    connect(loader, SIGNAL(downloadComplete(QByteArray, QUrl)),
            request, SIGNAL(downloadComplete(QByteArray, QUrl)));
    connect(loader, SIGNAL(downloadComplete(QString, QUrl)),
            request, SIGNAL(downloadComplete(QString, QUrl)));
    connect(loader, SIGNAL(uploadProgress(int, QUrl)),
            request, SIGNAL(uploadProgress(int, QUrl)));
    connect(loader, SIGNAL(downloadProgress(int, QUrl)),
            request, SIGNAL(downloadProgress(int, QUrl)));
    connect(loader, SIGNAL(error(QString, QUrl)),
            request, SIGNAL(error(QString, QUrl)));
    connect(loader, SIGNAL(finished()), request, SIGNAL(finished()));

    //
    // Загружаем!
    //
    loader->loadAsync(request->m_request->urlToLoad(), request->m_request->urlReferer());
}

void NetworkQueue::stop(NetworkRequestInternal* _internal) {
    m_mtx.lock();
    if (m_inQueue.contains(_internal)) {
        //
        // Либо запрос еще в очереди
        // Тогда его нужно оттуда удалить
        //
        m_queue.removeAll(_internal);
        m_inQueue.remove(_internal);
    }
    else {
        //
        // Либо запрос уже обрабатывается
        //
        for (auto iter = m_busyLoaders.begin(); iter != m_busyLoaders.end(); ++iter) {
            //
            // Найдем запрос в списке обрабатывающихся
            //
            if (iter.value() == _internal) {

                //
                // Отключим все сигналы
                // Обязательно сначала отключить сигналы, а затем остановить. Не наоборот!
                //
                disconnect(iter.key(), SIGNAL(downloadComplete(QByteArray)), iter.value(), SLOT(downloadComplete(QByteArray)));
                disconnect(iter.key(), SIGNAL(downloadComplete(QString)), iter.value(), SLOT(downloadComplete(QString)));
                disconnect(iter.key(), SIGNAL(uploadProgress(int)), iter.value(), SLOT(uploadProgress(int)));
                disconnect(iter.key(), SIGNAL(downloadProgress(int)), iter.value(), SLOT(downloadProgress(int)));
                disconnect(iter.key(), SIGNAL(error(QString)), iter.value(), SLOT(error(QString)));
                disconnect(iter.key(), SIGNAL(finished()), iter.value(), SLOT(finished()));

                //
                // Остановим запрос
                //
                iter.key()->stop();

                //
                // Удалим из списка используемых
                // К списку свободных WebLoader'ов припишет слот downloadComplete
                //
                m_busyLoaders.erase(iter);

                break;
            }
        }
    }
    m_mtx.unlock();
}

void NetworkQueue::setLoaderParams(WebLoader* _loader, NetworkRequestInternal* request) {
    _loader->setCookieJar(request->m_cookieJar);
    _loader->setRequestMethod(request->m_method);
    _loader->setLoadingTimeout(request->m_loadingTimeout);
    _loader->setWebRequest(request->m_request);
}

void NetworkQueue::downloadComplete(WebLoader* _loader) {
    m_mtx.lock();
    if (m_busyLoaders.contains(_loader)) {
        //
        // Если запрос отработал до конца (не был прерван методом stop),
        // то необходимо отключить сигналы
        // и удалить из списка используемых
        //
        NetworkRequestInternal* request = m_busyLoaders[_loader];

        disconnect(_loader, SIGNAL(downloadComplete(QByteArray, QUrl)),
                   request, SLOT(downloadComplete(QByteArray, QUrl)));
        disconnect(_loader, SIGNAL(downloadComplete(QString, QUrl)),
                   request, SLOT(downloadComplete(QString, QUrl)));
        disconnect(_loader, SIGNAL(uploadProgress(int, QUrl)),
                   request, SLOT(uploadProgress(int, QUrl)));
        disconnect(_loader, SIGNAL(downloadProgress(int, QUrl)),
                   request, SLOT(downloadProgress(int, QUrl)));
        disconnect(_loader, SIGNAL(error(QString, QUrl)),
                   request, SLOT(error(QString, QUrl)));
        disconnect(_loader, SIGNAL(finished()), request, SLOT(finished()));

        m_busyLoaders.remove(_loader);
    }

    //
    // Добавляем WebLoader в список свободных
    //
    m_freeLoaders.push_back(_loader);

    //
    //Смотрим, надо ли что еще выполнить из очереди
    //
    if (!m_queue.empty()) {
        pop();
    }
    m_mtx.unlock();
}
