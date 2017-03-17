#include "manager.h"

#include <QHBoxLayout>
#include <QDebug>

#include "container.h"
#include "listview.h"
#include "delegate.h"
#include "model.h"
#include "common.h"

#include "audioprovider.h"
#include "brightnessprovider.h"
#include "kblayoutprovider.h"
#include "displaymodeprovider.h"
#include "indicatorprovider.h"

Manager::Manager(QObject *parent)
    : QObject(parent),
      m_container(new Container),
      m_listview(new ListView),
      m_delegate(new Delegate),
      m_model(new Model(this)),
      m_currentProvider(nullptr),
      m_timer(new QTimer)
{
    m_timer->setSingleShot(true);
    m_timer->setInterval(2000);

    m_listview->setItemDelegate(m_delegate);
    m_listview->setModel(m_model);
    m_container->setContent(m_listview);

    m_providers << new AudioProvider(this) << new BrightnessProvider(this);
    m_providers << new KBLayoutProvider(this) << new DisplayModeProvider(this);
    m_providers << new IndicatorProvider(this);

    connect(m_timer, &QTimer::timeout, this, [this] {
        m_container->hide();
        m_currentProvider = nullptr;
    });
}

void Manager::ShowOSD(const QString &osd)
{
    bool repeat = false;

    for (AbstractOSDProvider *provider : m_providers) {
        qDebug() << provider << osd << provider->match(osd);
        if (provider->match(osd)) {
            repeat = (m_currentProvider == provider);
            m_currentProvider = provider;
            connect(provider, &AbstractOSDProvider::dataChanged, this, &Manager::updateUI);
        } else {
            disconnect(provider);
        }
    }

    if (m_currentProvider && repeat && m_container->isVisible()) {
        m_currentProvider->highlightNext();
    } else {
        m_currentProvider->highlightCurrent();
    }

    updateUI();

    if (m_currentProvider->checkConditions()) {
        m_container->show();
        m_timer->start();
    }
}

void Manager::updateUI()
{
    if (!m_currentProvider) return;

    m_model->setProvider(m_currentProvider);
    m_delegate->setProvider(m_currentProvider);
    m_listview->setFlow(m_currentProvider->flow());
    m_container->setContentsMargins(m_currentProvider->contentMargins());
    m_container->setFixedSize(m_currentProvider->contentSize());
    m_container->moveToCenter();
}
